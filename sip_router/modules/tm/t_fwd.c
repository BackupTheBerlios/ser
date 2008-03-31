/*
 * $Id: t_fwd.c,v 1.104 2008/03/31 18:19:50 bpintea Exp $
 *
 *
 * Copyright (C) 2001-2003 FhG Fokus
 *
 * This file is part of ser, a free SIP server.
 *
 * ser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * For a license to use the ser software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact iptel.org by e-mail at the following addresses:
 *    info@iptel.org
 *
 * ser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * History:
 * -------
 *  2003-02-13  proto support added (andrei)
 *  2003-02-24  s/T_NULL/T_NULL_CELL/ to avoid redefinition conflict w/
 *              nameser_compat.h (andrei)
 *  2003-03-01  kr set through a function now (jiri)
 *  2003-03-06  callbacks renamed; "blind UAC" introduced, which makes
 *              transaction behave as if it was forwarded even if it was
 *              not -- good for local UAS, like VM (jiri)
 *  2003-03-19  replaced all the mallocs/frees w/ pkg_malloc/pkg_free (andrei)
 *  2003-03-30  we now watch downstream delivery and if it fails, send an
 *              error message upstream (jiri)
 *  2003-04-14  use protocol from uri (jiri)
 *  2003-12-04  global TM callbacks switched to per transaction callbacks
 *              (bogdan)
 *  2004-02-13: t->is_invite and t->local replaced with flags (bogdan)
 *  2005-08-04  msg->parsed_uri and parsed_uri_ok are no saved & restored
 *               before & after handling the branches (andrei)
 *  2005-12-11  onsend_route support added for forwarding (andrei)
 *  2006-01-27  t_forward_no_ack will return error if a forward on an 
 *              already canceled transaction is attempted (andrei)
 *  2006-02-07  named routes support (andrei)
 *  2006-04-18  add_uac simplified + switched to struct dest_info (andrei)
 *  2006-04-20  pint_uac_request uses now struct dest_info (andrei)
 *  2006-08-11  dns failover support (andrei)
 *              t_forward_non_ack won't start retransmission on send errors
 *               anymore (WARNING: callers should release/kill the transaction
 *               if error is returned) (andrei)
 *  2006-09-15  e2e_cancel uses t_reply_unsafe when called from the 
 *               failure_route and replying to a cancel (andrei)
 *  2006-10-10  e2e_cancel update for the new/modified 
 *               which_cancel()/should_cancel() (andrei)
 *  2006-10-11  don't fork a new branch if the transaction or branch was
 *               canceled, or a 6xx was received
 *              stop retr. timers fix on cancel for non-invites     (andrei)
 *  2006-11-20  new_uri is no longer saved/restore across add_uac calls, since
 *              print_uac_request is now uri safe (andrei)
 *  2007-03-15  TMCB_ONSEND hooks added (andrei)
 *  2007-05-02  added t_forward_cancel(unmatched_cancel) (andrei)
 *  2007-05-24  added TMCB_E2ECANCEL_IN hook support (andrei)
 *  2007-05-28: e2e_cancel_branch() constructs the CANCEL from the
 *              outgoing INVITE instead of applying the lumps to the
 *              incomming one. (it can be disabled with reparse_invite=0) (Miklos)
 *              t_relay_cancel() introduced -- can be used to relay CANCELs
 *              at the beginning of the script. (Miklos)
 * 2007-06-04  running transaction are canceled hop by hop (andrei)
 *  2007-08-37  In case of DNS failover the new SIP message is constructed
 *              from the message buffer of the failed branch instead of
 *              applying the lumps again, because the per-branch lumps are not saved,
 *              thus, are not available. Set reparse_on_dns_failover to 0 to
 *              revert the change. (Miklos)
 */

#include "defs.h"


#include "../../dprint.h"
#include "../../config.h"
#include "../../parser/parser_f.h"
#include "../../ut.h"
#include "../../timer.h"
#include "../../hash_func.h"
#include "../../globals.h"
#include "../../cfg_core.h"
#include "../../mem/mem.h"
#include "../../dset.h"
#include "../../action.h"
#include "../../data_lump.h"
#include "../../onsend.h"
#include "../../compiler_opt.h"
#include "t_funcs.h"
#include "t_hooks.h"
#include "t_msgbuilder.h"
#include "ut.h"
#include "t_cancel.h"
#include "t_lookup.h"
#include "t_fwd.h"
#include "fix_lumps.h"
#include "config.h"
#ifdef USE_DNS_FAILOVER
#include "../../dns_cache.h"
#include "../../cfg_core.h" /* cfg_get(core, core_cfg, use_dns_failover) */
#include "../../msg_translator.h"
#include "lw_parser.h"
#endif
#ifdef USE_DST_BLACKLIST
#include "../../dst_blacklist.h"
#endif
#include "../../select_buf.h" /* reset_static_buffer() */
#ifdef POSTPONE_MSG_CLONING
#include "../../atomic_ops.h" /* membar_depends() */
#endif

static int goto_on_branch = 0, branch_route = 0;

void t_on_branch( unsigned int go_to )
{
	struct cell *t = get_t();

       /* in MODE_REPLY and MODE_ONFAILURE T will be set to current transaction;
        * in MODE_REQUEST T will be set only if the transaction was already
        * created; if not -> use the static variable */
	if (!t || t==T_UNDEFINED ) {
		goto_on_branch=go_to;
	} else {
		get_t()->on_branch = go_to;
	}
}

unsigned int get_on_branch(void)
{
	return goto_on_branch;
}


static char *print_uac_request( struct cell *t, struct sip_msg *i_req,
	int branch, str *uri, unsigned int *len, struct dest_info* dst)
{
	char *buf, *shbuf;
	str* msg_uri;
	struct lump* add_rm_backup, *body_lumps_backup;
	struct sip_uri parsed_uri_bak;
	int parsed_uri_ok_bak, uri_backed_up;
	str msg_uri_bak;
	struct run_act_ctx ra_ctx;

	shbuf=0;
	msg_uri_bak.s=0; /* kill warnings */
	msg_uri_bak.len=0;
	parsed_uri_ok_bak=0;
	uri_backed_up=0;

	/* ... we calculate branch ... */	
	if (!t_calc_branch(t, branch, i_req->add_to_branch_s,
			&i_req->add_to_branch_len ))
	{
		LOG(L_ERR, "ERROR: print_uac_request: branch computation failed\n");
		goto error00;
	}

	/* ... update uri ... */
	msg_uri=GET_RURI(i_req);
	if ((msg_uri->s!=uri->s) || (msg_uri->len!=uri->len)){
		msg_uri_bak=i_req->new_uri;
		parsed_uri_ok_bak=i_req->parsed_uri_ok;
		parsed_uri_bak=i_req->parsed_uri;
		i_req->new_uri=*uri;
		i_req->parsed_uri_ok=0;
		uri_backed_up=1;
	}

#ifdef POSTPONE_MSG_CLONING
	/* lumps can be set outside of the lock, make sure that we read
	 * the up-to-date values */
	membar_depends();
#endif
	add_rm_backup = i_req->add_rm;
	body_lumps_backup = i_req->body_lumps;
	i_req->add_rm = dup_lump_list(i_req->add_rm);
	i_req->body_lumps = dup_lump_list(i_req->body_lumps);

	if (unlikely(branch_route)) {
		reset_static_buffer();
		     /* run branch_route actions if provided */
		init_run_actions_ctx(&ra_ctx);
		if (run_actions(&ra_ctx, branch_rt.rlist[branch_route], i_req) < 0) {
			LOG(L_ERR, "ERROR: print_uac_request: Error in run_actions\n");
               }
	}

	/* run the specific callbacks for this transaction */
	if (unlikely(has_tran_tmcbs(t, TMCB_REQUEST_FWDED)))
		run_trans_callbacks( TMCB_REQUEST_FWDED , t, i_req, 0,
								-i_req->REQ_METHOD);

	/* ... and build it now */
	buf=build_req_buf_from_sip_req( i_req, len, dst);
#ifdef DBG_MSG_QA
	if (buf[*len-1]==0) {
		LOG(L_ERR, "ERROR: print_uac_request: sanity check failed\n");
		abort();
	}
#endif
	if (!buf) {
		LOG(L_ERR, "ERROR: print_uac_request: no pkg_mem\n"); 
		ser_error=E_OUT_OF_MEM;
		goto error01;
	}

	shbuf=(char *)shm_malloc(*len);
	if (!shbuf) {
		ser_error=E_OUT_OF_MEM;
		LOG(L_ERR, "ERROR: print_uac_request: no shmem\n");
		goto error02;
	}
	memcpy( shbuf, buf, *len );

error02:
	pkg_free( buf );
error01:
	     /* Delete the duplicated lump lists, this will also delete
	      * all lumps created here, such as lumps created in per-branch
	      * routing sections, Via, and Content-Length headers created in
	      * build_req_buf_from_sip_req
	      */
	free_duped_lump_list(i_req->add_rm);
	free_duped_lump_list(i_req->body_lumps);
	     /* Restore the lists from backups */
	i_req->add_rm = add_rm_backup;
	i_req->body_lumps = body_lumps_backup;
	/* restore the new_uri from the backup */
	if (uri_backed_up){
		i_req->new_uri=msg_uri_bak;
		i_req->parsed_uri=parsed_uri_bak;
		i_req->parsed_uri_ok=parsed_uri_ok_bak;
	}

 error00:
	return shbuf;
}

#ifdef USE_DNS_FAILOVER
/* Similar to print_uac_request(), but this function uses the outgoing message buffer of
   the failed branch to construt the new message in case of DNS failover.

   WARNING: only the first VIA header is replaced in the buffer, the rest
   of the message is untuched, thus, the send socket is corrected only in the VIA HF.
*/
static char *print_uac_request_from_buf( struct cell *t, struct sip_msg *i_req,
	int branch, str *uri, unsigned int *len, struct dest_info* dst,
	char *buf, short buf_len)
{
	char *shbuf;
	str branch_str;
	char *via, *old_via_begin, *old_via_end;
	unsigned int via_len;

	shbuf=0;

	/* ... we calculate branch ... */	
	if (!t_calc_branch(t, branch, i_req->add_to_branch_s,
			&i_req->add_to_branch_len ))
	{
		LOG(L_ERR, "ERROR: print_uac_request_from_buf: branch computation failed\n");
		goto error00;
	}
	branch_str.s = i_req->add_to_branch_s;
	branch_str.len = i_req->add_to_branch_len;

	/* find the beginning of the first via header in the buffer */
	old_via_begin = lw_find_via(buf, buf+buf_len);
	if (!old_via_begin) {
		LOG(L_ERR, "ERROR: print_uac_request_from_buf: beginning of via header not found\n");
		goto error00;
	}
	/* find the end of the first via header in the buffer */
	old_via_end = lw_next_line(old_via_begin, buf+buf_len);
	if (!old_via_end) {
		LOG(L_ERR, "ERROR: print_uac_request_from_buf: end of via header not found\n");
		goto error00;
	}

	/* create the new VIA HF */
	via = create_via_hf(&via_len, i_req, dst, &branch_str);
	if (!via) {
		LOG(L_ERR, "ERROR: print_uac_request_from_buf: via building failed\n");
		goto error00;
	}

	/* allocate memory for the new buffer */
	*len = buf_len + via_len - (old_via_end - old_via_begin);
	shbuf=(char *)shm_malloc(*len);
	if (!shbuf) {
		ser_error=E_OUT_OF_MEM;
		LOG(L_ERR, "ERROR: print_uac_request_from_buf: no shmem\n");
		goto error01;
	}

	/* construct the new buffer */
	memcpy(shbuf, buf, old_via_begin-buf);
	memcpy(shbuf+(old_via_begin-buf), via, via_len);
	memcpy(shbuf+(old_via_begin-buf)+via_len, old_via_end, (buf+buf_len)-old_via_end);

#ifdef DBG_MSG_QA
	if (shbuf[*len-1]==0) {
		LOG(L_ERR, "ERROR: print_uac_request_from_buf: sanity check failed\n");
		abort();
	}
#endif

error01:
	pkg_free(via);
error00:
	return shbuf;
}
#endif

/* introduce a new uac, which is blind -- it only creates the
   data structures and starts FR timer, but that's it; it does
   not print messages and send anything anywhere; that is good
   for FIFO apps -- the transaction must look operationally
   and FR must be ticking, whereas the request is "forwarded"
   using a non-SIP way and will be replied the same way
*/
int add_blind_uac( /*struct cell *t*/ )
{
	unsigned short branch;
	struct cell *t;

	t=get_t();
	if (t==T_UNDEFINED || !t ) {
		LOG(L_ERR, "ERROR: add_blind_uac: no transaction context\n");
		return -1;
	}

	branch=t->nr_of_outgoings;	
	if (branch==MAX_BRANCHES) {
		LOG(L_ERR, "ERROR: add_blind_uac: "
			"maximum number of branches exceeded\n");
		return -1;
	}
	/* make sure it will be replied */
	t->flags |= T_NOISY_CTIMER_FLAG;
	membar_write(); /* to allow lockless which_cancel() we want to be sure 
					   all the writes finished before updating branch number*/
	t->nr_of_outgoings=(branch+1);
	/* start FR timer -- protocol set by default to PROTO_NONE,
       which means retransmission timer will not be started
    */
	if (start_retr(&t->uac[branch].request)!=0)
		LOG(L_CRIT, "BUG: add_blind_uac: start retr failed for %p\n",
				&t->uac[branch].request);
	/* we are on a timer -- don't need to put on wait on script
	   clean-up	
	*/
	set_kr(REQ_FWDED); 

	return 1; /* success */
}

/* introduce a new uac to transaction; returns its branch id (>=0)
   or error (<0); it doesn't send a message yet -- a reply to it
   might interfere with the processes of adding multiple branches;
   On error returns <0 & sets ser_error to the same value
*/
int add_uac( struct cell *t, struct sip_msg *request, str *uri, str* next_hop,
	struct proxy_l *proxy, int proto )
{

	int ret;
	unsigned short branch;
	char *shbuf;
	unsigned int len;

	branch=t->nr_of_outgoings;
	if (branch==MAX_BRANCHES) {
		LOG(L_ERR, "ERROR: add_uac: maximum number of branches exceeded\n");
		ret=ser_error=E_TOO_MANY_BRANCHES;
		goto error;
	}

	/* check existing buffer -- rewriting should never occur */
	if (t->uac[branch].request.buffer) {
		LOG(L_CRIT, "ERROR: add_uac: buffer rewrite attempt\n");
		ret=ser_error=E_BUG;
		goto error;
	}

	/* check DNS resolution */
	if (proxy){
		/* dst filled from the proxy */
		init_dest_info(&t->uac[branch].request.dst);
		t->uac[branch].request.dst.proto=get_proto(proto, proxy->proto);
		proxy2su(&t->uac[branch].request.dst.to, proxy);
		/* fill dst send_sock */
		t->uac[branch].request.dst.send_sock =
		get_send_socket( request, &t->uac[branch].request.dst.to,
								t->uac[branch].request.dst.proto);
	}else {
#ifdef USE_DNS_FAILOVER
		if (uri2dst(&t->uac[branch].dns_h, &t->uac[branch].request.dst,
					request, next_hop?next_hop:uri, proto) == 0)
#else
		/* dst filled from the uri & request (send_socket) */
		if (uri2dst(&t->uac[branch].request.dst, request,
						next_hop ? next_hop: uri, proto)==0)
#endif
		{
			ret=ser_error=E_BAD_ADDRESS;
			goto error;
		}
	}
	
	/* check if send_sock is ok */
	if (t->uac[branch].request.dst.send_sock==0) {
		LOG(L_ERR, "ERROR: add_uac: can't fwd to af %d, proto %d "
			" (no corresponding listening socket)\n",
			t->uac[branch].request.dst.to.s.sa_family, 
			t->uac[branch].request.dst.proto );
		ret=ser_error=E_NO_SOCKET;
		goto error01;
	}

	/* now message printing starts ... */
	shbuf=print_uac_request( t, request, branch, uri, 
							&len, &t->uac[branch].request.dst);
	if (!shbuf) {
		ret=ser_error=E_OUT_OF_MEM;
		goto error01;
	}

	/* things went well, move ahead and install new buffer! */
	t->uac[branch].request.buffer=shbuf;
	t->uac[branch].request.buffer_len=len;
	t->uac[branch].uri.s=t->uac[branch].request.buffer+
		request->first_line.u.request.method.len+1;
	t->uac[branch].uri.len=uri->len;
	membar_write(); /* to allow lockless ops (e.g. which_cancel()) we want
					   to be sure everything above is fully written before
					   updating branches no. */
	t->nr_of_outgoings=(branch+1);

	/* update stats */
	if (proxy){
		proxy_mark(proxy, 1);
	}
	/* done! */
	ret=branch;
		
error01:
error:
	return ret;
}



#ifdef USE_DNS_FAILOVER
/* Similar to add_uac(), but this function uses the outgoing message buffer of
   the failed branch to construt the new message in case of DNS failover.
*/
static int add_uac_from_buf( struct cell *t, struct sip_msg *request, str *uri, int proto,
			char *buf, short buf_len)
{

	int ret;
	unsigned short branch;
	char *shbuf;
	unsigned int len;

	branch=t->nr_of_outgoings;
	if (branch==MAX_BRANCHES) {
		LOG(L_ERR, "ERROR: add_uac_from_buf: maximum number of branches exceeded\n");
		ret=ser_error=E_TOO_MANY_BRANCHES;
		goto error;
	}

	/* check existing buffer -- rewriting should never occur */
	if (t->uac[branch].request.buffer) {
		LOG(L_CRIT, "ERROR: add_uac_from_buf: buffer rewrite attempt\n");
		ret=ser_error=E_BUG;
		goto error;
	}

	if (uri2dst(&t->uac[branch].dns_h, &t->uac[branch].request.dst,
				request, uri, proto) == 0)
	{
		ret=ser_error=E_BAD_ADDRESS;
		goto error;
	}
	
	/* check if send_sock is ok */
	if (t->uac[branch].request.dst.send_sock==0) {
		LOG(L_ERR, "ERROR: add_uac_from_buf: can't fwd to af %d, proto %d "
			" (no corresponding listening socket)\n",
			t->uac[branch].request.dst.to.s.sa_family, 
			t->uac[branch].request.dst.proto );
		ret=ser_error=E_NO_SOCKET;
		goto error;
	}

	/* now message printing starts ... */
	shbuf=print_uac_request_from_buf( t, request, branch, uri, 
							&len, &t->uac[branch].request.dst,
							buf, buf_len);
	if (!shbuf) {
		ret=ser_error=E_OUT_OF_MEM;
		goto error;
	}

	/* things went well, move ahead and install new buffer! */
	t->uac[branch].request.buffer=shbuf;
	t->uac[branch].request.buffer_len=len;
	t->uac[branch].uri.s=t->uac[branch].request.buffer+
		request->first_line.u.request.method.len+1;
	t->uac[branch].uri.len=uri->len;
	membar_write(); /* to allow lockless ops (e.g. which_cancel()) we want
					   to be sure everything above is fully written before
					   updating branches no. */
	t->nr_of_outgoings=(branch+1);

	/* done! */
	ret=branch;
		
error:
	return ret;
}

/* introduce a new uac to transaction, based on old_uac and a possible
 *  new ip address (if the dns name resolves to more ips). If no more
 *   ips are found => returns -1.
 *  returns its branch id (>=0)
   or error (<0) and sets ser_error if needed; it doesn't send a message 
   yet -- a reply to it
   might interfere with the processes of adding multiple branches
   if lock_replies is 1 replies will be locked for t until the new branch
   is added (to prevent add branches races). Use 0 if the reply lock is
   already held, e.g. in failure route/handlers (WARNING: using 1 in a 
   failure route will cause a deadlock).
*/
int add_uac_dns_fallback( struct cell *t, struct sip_msg* msg, 
									struct ua_client* old_uac,
									int lock_replies)
{
	int ret;
	
	ret=-1;
	if (cfg_get(core, core_cfg, use_dns_failover) && 
			!((t->flags & T_DONT_FORK) || uac_dont_fork(old_uac)) &&
			dns_srv_handle_next(&old_uac->dns_h, 0)){
			if (lock_replies){
				/* use reply lock to guarantee nobody is adding a branch
				 * in the same time */
				LOCK_REPLIES(t);
				/* check again that we can fork */
				if ((t->flags & T_DONT_FORK) || uac_dont_fork(old_uac)){
					UNLOCK_REPLIES(t);
					DBG("add_uac_dns_fallback: no forking on => no new"
							" branches\n");
					return ret;
				}
			}
			if (t->nr_of_outgoings >= MAX_BRANCHES){
				LOG(L_ERR, "ERROR: add_uac_dns_fallback: maximum number of "
							"branches exceeded\n");
				if (lock_replies)
					UNLOCK_REPLIES(t);
					ret=ser_error=E_TOO_MANY_BRANCHES;
				return ret;
			}
			/* copy the dns handle into the new uac */
			dns_srv_handle_cpy(&t->uac[t->nr_of_outgoings].dns_h,
								&old_uac->dns_h);

			if (cfg_get(tm, tm_cfg, reparse_on_dns_failover))
				/* Reuse the old buffer and only replace the via header.
				 * The drowback is that the send_socket is not corrected
				 * in the rest of the message, only in the VIA HF (Miklos) */
				ret=add_uac_from_buf(t,  msg, &old_uac->uri, 
							old_uac->request.dst.proto,
							old_uac->request.buffer,
							old_uac->request.buffer_len);
			else
				/* add_uac will use dns_h => next_hop will be ignored.
				 * Unfortunately we can't reuse the old buffer, the branch id
				 *  must be changed and the send_socket might be different =>
				 *  re-create the whole uac */
				ret=add_uac(t,  msg, &old_uac->uri, 0, 0, 
							old_uac->request.dst.proto);

			if (ret<0){
				/* failed, delete the copied dns_h */
				dns_srv_handle_put(&t->uac[t->nr_of_outgoings].dns_h);
			}
			if (lock_replies){
				UNLOCK_REPLIES(t);
			}
	}
	return ret;
}

#endif

int e2e_cancel_branch( struct sip_msg *cancel_msg, struct cell *t_cancel, 
	struct cell *t_invite, int branch )
{
	int ret;
	char *shbuf;
	unsigned int len;

	ret=-1;
	if (t_cancel->uac[branch].request.buffer) {
		LOG(L_CRIT, "ERROR: e2e_cancel_branch: buffer rewrite attempt\n");
		ret=ser_error=E_BUG;
		goto error;
	}
	if (t_invite->uac[branch].request.buffer==0){
		/* inactive / deleted  branch */
		goto error;
	}
	t_invite->uac[branch].request.flags|=F_RB_CANCELED;

	/* note -- there is a gap in proxy stats -- we don't update 
	   proxy stats with CANCEL (proxy->ok, proxy->tx, etc.)
	*/

	/* print */
	if (cfg_get(tm, tm_cfg, reparse_invite)) {
		/* buffer is built localy from the INVITE which was sent out */
#ifdef POSTPONE_MSG_CLONING
		/* lumps can be set outside of the lock, make sure that we read
		 * the up-to-date values */
		membar_depends();
#endif
		if (cancel_msg->add_rm || cancel_msg->body_lumps) {
			LOG(L_WARN, "WARNING: e2e_cancel_branch: CANCEL is built locally, "
			"thus lumps are not applied to the message!\n");
		}
		shbuf=build_local_reparse( t_invite, branch, &len, CANCEL, CANCEL_LEN, &t_invite->to);

	} else {
		/* buffer is constructed from the received CANCEL with applying lumps */
		shbuf=print_uac_request( t_cancel, cancel_msg, branch, 
							&t_invite->uac[branch].uri, &len, 
							&t_invite->uac[branch].request.dst);
	}

	if (!shbuf) {
		LOG(L_ERR, "ERROR: e2e_cancel_branch: printing e2e cancel failed\n");
		ret=ser_error=E_OUT_OF_MEM;
		goto error;
	}
	
	/* install buffer */
	t_cancel->uac[branch].request.dst=t_invite->uac[branch].request.dst;
	t_cancel->uac[branch].request.buffer=shbuf;
	t_cancel->uac[branch].request.buffer_len=len;
	t_cancel->uac[branch].uri.s=t_cancel->uac[branch].request.buffer+
		cancel_msg->first_line.u.request.method.len+1;
	t_cancel->uac[branch].uri.len=t_invite->uac[branch].uri.len;
	

	/* success */
	ret=1;


error:
	return ret;
}

void e2e_cancel( struct sip_msg *cancel_msg, 
	struct cell *t_cancel, struct cell *t_invite )
{
	branch_bm_t cancel_bm;
#ifndef E2E_CANCEL_HOP_BY_HOP
	branch_bm_t tmp_bm;
#endif
	int i;
	int lowest_error;
	int ret;
	struct tmcb_params tmcb;

	cancel_bm=0;
	lowest_error=0;

	if (unlikely(has_tran_tmcbs(t_invite, TMCB_E2ECANCEL_IN))){
		INIT_TMCB_PARAMS(tmcb, cancel_msg, 0, cancel_msg->REQ_METHOD);
		run_trans_callbacks_internal(&t_invite->tmcb_hl, TMCB_E2ECANCEL_IN, 
										t_invite, &tmcb);
	}
	/* first check if there are any branches */
	if (t_invite->nr_of_outgoings==0){
		t_invite->flags|=T_CANCELED;
		/* no branches yet => force a reply to the invite */
		t_reply( t_invite, t_invite->uas.request, 487, CANCELED );
		DBG("DEBUG: e2e_cancel: e2e cancel -- no more pending branches\n");
		t_reply( t_cancel, cancel_msg, 200, CANCEL_DONE );
		return;
	}
	
	/* determine which branches to cancel ... */
	which_cancel( t_invite, &cancel_bm );
#ifdef E2E_CANCEL_HOP_BY_HOP
	/* we don't need to set t_cancel label to be the same as t_invite if
	 * we do hop by hop cancel. The cancel transaction will have a different 
	 * label, but this is not a problem since this transaction is only used to
	 * send a reply back. The cancels sent upstream will be part of the invite
	 * transaction (local_cancel retr. bufs) and they will be generated with
	 * the same via as the invite.
	 * Note however that setting t_cancel label the same as t_invite will work
	 * too (the upstream cancel replies will properly match the t_invite
	 * transaction and will not match the t_cancel because t_cancel will always
	 * have 0 branches and we check for the branch number in 
	 * t_reply_matching() ).
	 */
	for (i=0; i<t_invite->nr_of_outgoings; i++)
		if (cancel_bm & (1<<i)) {
			/* it's safe to get the reply lock since e2e_cancel is
			 * called with the cancel as the "current" transaction so
			 * at most t_cancel REPLY_LOCK is held in this process =>
			 * no deadlock possibility */
			ret=cancel_branch(t_invite, i, cfg_get(tm,tm_cfg, cancel_b_flags));
			if (ret<0) cancel_bm &= ~(1<<i);
			if (ret<lowest_error) lowest_error=ret;
		}
#else /* ! E2E_CANCEL_HOP_BY_HOP */
	/* fix label -- it must be same for reply matching (the label is part of
	 * the generated via branch for the cancels sent upstream and if it
	 * would be different form the one in the INVITE the transactions would not
	 * match */
	t_cancel->label=t_invite->label;
	t_cancel->nr_of_outgoings=t_invite->nr_of_outgoings;
	/* ... and install CANCEL UACs */
	for (i=0; i<t_invite->nr_of_outgoings; i++)
		if ((cancel_bm & (1<<i)) && (t_invite->uac[i].last_received>=100)) {
			ret=e2e_cancel_branch(cancel_msg, t_cancel, t_invite, i);
			if (ret<0) cancel_bm &= ~(1<<i);
			if (ret<lowest_error) lowest_error=ret;
		}

	/* send them out */
	for (i = 0; i < t_cancel->nr_of_outgoings; i++) {
		if (cancel_bm & (1 << i)) {
			if (t_invite->uac[i].last_received>=100){
				/* Provisional reply received on this branch, send CANCEL */
				/* we do need to stop the retr. timers if the request is not 
				 * an invite and since the stop_rb_retr() cost is lower then
				 * the invite check we do it always --andrei */
				stop_rb_retr(&t_invite->uac[i].request);
				if (SEND_BUFFER(&t_cancel->uac[i].request) == -1) {
					LOG(L_ERR, "ERROR: e2e_cancel: send failed\n");
				}
#ifdef TMCB_ONSEND
				else{
					if (unlikely(has_tran_tmcbs(t_cancel, TMCB_REQUEST_SENT)))
						run_onsend_callbacks(TMCB_REQUEST_SENT, 
												&t_cancel->uac[i].request,
												cancel_msg, 0, TMCB_LOCAL_F);
				}
#endif
				if (start_retr( &t_cancel->uac[i].request )!=0)
					LOG(L_CRIT, "BUG: e2e_cancel: failed to start retr."
							" for %p\n", &t_cancel->uac[i].request);
			} else {
				/* No provisional response received, stop
				 * retransmission timers */
				if (!(cfg_get(tm, tm_cfg, cancel_b_flags) & 
							F_CANCEL_B_FORCE_RETR))
					stop_rb_retr(&t_invite->uac[i].request);
				/* no need to stop fr, it will be stoped by relay_reply
				 * put_on_wait -- andrei */
				/* Generate faked reply */
				if (cfg_get(tm, tm_cfg, cancel_b_flags) &
						F_CANCEL_B_FAKE_REPLY){
					LOCK_REPLIES(t_invite);
					if (relay_reply(t_invite, FAKED_REPLY, i, 487, &tmp_bm) == 
							RPS_ERROR) {
						lowest_error = -1;
					}
				}
			}
		}
	}
#endif /*E2E_CANCEL_HOP_BY_HOP */

	/* if error occurred, let it know upstream (final reply
	   will also move the transaction on wait state
	*/
	if (lowest_error<0) {
		LOG(L_ERR, "ERROR: cancel error\n");
		/* if called from failure_route, make sure that the unsafe version
		 * is called (we are already holding the reply mutex for the cancel
		 * transaction).
		 */
		if ((rmode==MODE_ONFAILURE) && (t_cancel==get_t()))
			t_reply_unsafe( t_cancel, cancel_msg, 500, "cancel error");
		else
			t_reply( t_cancel, cancel_msg, 500, "cancel error");
	} else if (cancel_bm) {
		/* if there are pending branches, let upstream know we
		   are working on it
		*/
		DBG("DEBUG: e2e_cancel: e2e cancel proceeding\n");
		/* if called from failure_route, make sure that the unsafe version
		 * is called (we are already hold the reply mutex for the cancel
		 * transaction).
		 */
		if ((rmode==MODE_ONFAILURE) && (t_cancel==get_t()))
			t_reply_unsafe( t_cancel, cancel_msg, 200, CANCELING );
		else
			t_reply( t_cancel, cancel_msg, 200, CANCELING );
	} else {
		/* if the transaction exists, but there are no more pending
		   branches, tell upstream we're done
		*/
		DBG("DEBUG: e2e_cancel: e2e cancel -- no more pending branches\n");
		/* if called from failure_route, make sure that the unsafe version
		 * is called (we are already hold the reply mutex for the cancel
		 * transaction).
		 */
		if ((rmode==MODE_ONFAILURE) && (t_cancel==get_t()))
			t_reply_unsafe( t_cancel, cancel_msg, 200, CANCEL_DONE );
		else
			t_reply( t_cancel, cancel_msg, 200, CANCEL_DONE );
	}
}



/* sends one uac/branch buffer and fallbacks to other ips if
 *  the destination resolves to several addresses
 *  Takes care of starting timers a.s.o. (on send success)
 *  returns: -2 on error, -1 on drop,  current branch id on success,
 *   new branch id on send error/blacklist, when failover is possible
 *    (ret>=0 && ret!=branch)
 *    if lock_replies is 1, the replies for t will be locked when adding
 *     new branches (to prevent races). Use 0 from failure routes or other
 *     places where the reply lock is already held, to avoid deadlocks. */
int t_send_branch( struct cell *t, int branch, struct sip_msg* p_msg ,
					struct proxy_l * proxy, int lock_replies)
{
	struct ip_addr ip; /* debugging */
	int ret;
	struct ua_client* uac;
	
	uac=&t->uac[branch];
	ret=branch;
	if (run_onsend(p_msg,	&uac->request.dst, uac->request.buffer,
					uac->request.buffer_len)==0){
		/* disable the current branch: set a "fake" timeout
		 *  reply code but don't set uac->reply, to avoid overriding 
		 *  a higly unlikely, perfectly timed fake reply (to a message
		 *   we never sent).
		 * (code=final reply && reply==0 => t_pick_branch won't ever pick it)*/
			uac->last_received=408;
			su2ip_addr(&ip, &uac->request.dst.to);
			DBG("t_send_branch: onsend_route dropped msg. to %s:%d (%d)\n",
							ip_addr2a(&ip), su_getport(&uac->request.dst.to),
							uac->request.dst.proto);
#ifdef USE_DNS_FAILOVER
			/* if the destination resolves to more ips, add another
			 *  branch/uac */
			if (cfg_get(core, core_cfg, use_dns_failover)){
				ret=add_uac_dns_fallback(t, p_msg, uac, lock_replies);
				if (ret>=0){
					su2ip_addr(&ip, &uac->request.dst.to);
					DBG("t_send_branch: send on branch %d failed "
							"(onsend_route), trying another ip %s:%d (%d)\n",
							branch, ip_addr2a(&ip),
							su_getport(&uac->request.dst.to),
							uac->request.dst.proto);
					/* success, return new branch */
					return ret;
				}
			}
#endif /* USE_DNS_FAILOVER*/
		return -1; /* drop, try next branch */
	}
#ifdef USE_DST_BLACKLIST
	if (cfg_get(core, core_cfg, use_dst_blacklist)
		&& p_msg
		&& (p_msg->REQ_METHOD & cfg_get(tm, tm_cfg, tm_blst_methods_lookup))
	){
		if (dst_is_blacklisted(&uac->request.dst, p_msg)){
			su2ip_addr(&ip, &uac->request.dst.to);
			DBG("t_send_branch: blacklisted destination: %s:%d (%d)\n",
							ip_addr2a(&ip), su_getport(&uac->request.dst.to),
							uac->request.dst.proto);
			/* disable the current branch: set a "fake" timeout
			 *  reply code but don't set uac->reply, to avoid overriding 
			 *  a higly unlikely, perfectly timed fake reply (to a message
			 *   we never sent).  (code=final reply && reply==0 => 
			 *   t_pick_branch won't ever pick it)*/
			uac->last_received=408;
#ifdef USE_DNS_FAILOVER
			/* if the destination resolves to more ips, add another
			 *  branch/uac */
			if (cfg_get(core, core_cfg, use_dns_failover)){
				ret=add_uac_dns_fallback(t, p_msg, uac, lock_replies);
				if (ret>=0){
					su2ip_addr(&ip, &uac->request.dst.to);
					DBG("t_send_branch: send on branch %d failed (blacklist),"
							" trying another ip %s:%d (%d)\n", branch,
							ip_addr2a(&ip), su_getport(&uac->request.dst.to),
							uac->request.dst.proto);
					/* success, return new branch */
					return ret;
				}
			}
#endif /* USE_DNS_FAILOVER*/
			return -1; /* don't send */
		}
	}
#endif /* USE_DST_BLACKLIST */
	if (SEND_BUFFER( &uac->request)==-1) {
		/* disable the current branch: set a "fake" timeout
		 *  reply code but don't set uac->reply, to avoid overriding 
		 *  a highly unlikely, perfectly timed fake reply (to a message
		 *  we never sent).
		 * (code=final reply && reply==0 => t_pick_branch won't ever pick it)*/
		uac->last_received=408;
		su2ip_addr(&ip, &uac->request.dst.to);
		DBG("t_send_branch: send to %s:%d (%d) failed\n",
							ip_addr2a(&ip), su_getport(&uac->request.dst.to),
							uac->request.dst.proto);
#ifdef USE_DST_BLACKLIST
		if (cfg_get(core, core_cfg, use_dst_blacklist))
			dst_blacklist_add(BLST_ERR_SEND, &uac->request.dst, p_msg);
#endif
#ifdef USE_DNS_FAILOVER
		/* if the destination resolves to more ips, add another
		 *  branch/uac */
		if (cfg_get(core, core_cfg, use_dns_failover)){
			ret=add_uac_dns_fallback(t, p_msg, uac, lock_replies);
			if (ret>=0){
				/* success, return new branch */
				DBG("t_send_branch: send on branch %d failed, adding another"
						" branch with another ip\n", branch);
				return ret;
			}
		}
#endif
		LOG(L_ERR, "ERROR: t_send_branch: sending request on branch %d "
				"failed\n", branch);
		if (proxy) { proxy->errors++; proxy->ok=0; }
		return -2;
	} else {
#ifdef TMCB_ONSEND
		if (unlikely(has_tran_tmcbs(t, TMCB_REQUEST_SENT)))
			run_onsend_callbacks(TMCB_REQUEST_SENT, &uac->request, p_msg, 0,0);
#endif
		/* start retr. only if the send succeeded */
		if (start_retr( &uac->request )!=0){
			LOG(L_CRIT, "BUG: t_send_branch: retr. already started for %p\n",
					&uac->request);
			return -2;
		}
	}
	return ret;
}



/* function returns:
 *       1 - forward successful
 *      -1 - error during forward
 */
int t_forward_nonack( struct cell *t, struct sip_msg* p_msg , 
	struct proxy_l * proxy, int proto)
{
	int branch_ret, lowest_ret;
	str current_uri;
	branch_bm_t	added_branches;
	int first_branch;
	int i, q;
	struct cell *t_invite;
	int success_branch;
	int try_new;
	int lock_replies;
	str dst_uri;
	struct socket_info* si, *backup_si;
	

	/* make -Wall happy */
	current_uri.s=0;

	if (t->flags & T_CANCELED){
		DBG("t_forward_non_ack: no forwarding on a canceled transaction\n");
		ser_error=E_CANCELED;
		return -1;
	}
	if (p_msg->REQ_METHOD==METHOD_CANCEL) { 
		t_invite=t_lookupOriginalT(  p_msg );
		if (t_invite!=T_NULL_CELL) {
			e2e_cancel( p_msg, t, t_invite );
			UNREF(t_invite);
			/* it should be set to REQ_RPLD by e2e_cancel, which should
			 * send a final reply */
			set_kr(REQ_FWDED);
			return 1;
		}
	}

	backup_si = p_msg->force_send_socket;
	/* if no more specific error code is known, use this */
	lowest_ret=E_UNSPEC;
	/* branches added */
	added_branches=0;
	/* branch to begin with */
	first_branch=t->nr_of_outgoings;

	if (t->on_branch) {
		/* tell add_uac that it should run branch route actions */
		branch_route = t->on_branch;
		/* reset the flag before running the actions (so that it
		 * could be set again in branch_route if needed
		 */
		t_on_branch(0);
	} else {
		branch_route = 0;
	}
	
	/* on first-time forwarding, use current uri, later only what
	   is in additional branches (which may be continuously refilled
	*/
	if (first_branch==0) {
#ifdef POSTPONE_MSG_CLONING
		/* update the shmem-ized msg with the lumps */
		if ((rmode == MODE_REQUEST) &&
			save_msg_lumps(t->uas.request, p_msg)) {
				LOG(L_ERR, "ERROR: t_forward_nonack: "
					"failed to save the message lumps\n");
				return -1;
			}
#endif
		try_new=1;
		branch_ret=add_uac( t, p_msg, GET_RURI(p_msg), GET_NEXT_HOP(p_msg),
							proxy, proto );
		if (branch_ret>=0) 
			added_branches |= 1<<branch_ret;
		else
			lowest_ret=MIN_int(lowest_ret, branch_ret);
	} else try_new=0;

	init_branch_iterator();
	while((current_uri.s=next_branch( &current_uri.len, &q, &dst_uri.s, &dst_uri.len, &si))) {
		try_new++;
		p_msg->force_send_socket = si;
		branch_ret=add_uac( t, p_msg, &current_uri, 
				    (dst_uri.len) ? (&dst_uri) : &current_uri, 
				    proxy, proto);
		/* pick some of the errors in case things go wrong;
		   note that picking lowest error is just as good as
		   any other algorithm which picks any other negative
		   branch result */
		if (branch_ret>=0) 
			added_branches |= 1<<branch_ret;
		else
			lowest_ret=MIN_int(lowest_ret, branch_ret);
	}
	/* consume processed branches */
	clear_branches();

	p_msg->force_send_socket = backup_si;

	/* don't forget to clear all branches processed so far */

	/* things went wrong ... no new branch has been fwd-ed at all */
	if (added_branches==0) {
		if (try_new==0) {
			LOG(L_ERR, "ERROR: t_forward_nonack: no branches for"
						" forwarding\n");
			/* either failed to add branches, or there were no more branches
			*/
			ser_error=MIN_int(lowest_ret, E_CFG);
			return -1;
		}
		LOG(L_ERR, "ERROR: t_forward_nonack: failure to add branches\n");
		ser_error=lowest_ret;
		return lowest_ret;
	}
	ser_error=0; /* clear branch adding errors */
	/* send them out now */
	success_branch=0;
	lock_replies= ! ((rmode==MODE_ONFAILURE) && (t==get_t()));
	for (i=first_branch; i<t->nr_of_outgoings; i++) {
		if (added_branches & (1<<i)) {
			
			branch_ret=t_send_branch(t, i, p_msg , proxy, lock_replies);
			if (branch_ret>=0){ /* some kind of success */
				if (branch_ret==i) /* success */
					success_branch++;
				else /* new branch added */
					added_branches |= 1<<branch_ret;
			}
		}
	}
	if (success_branch<=0) {
		/* return always E_SEND for now
		 * (the real reason could be: denied by onsend routes, blacklisted,
		 *  send failed or any of the errors listed before + dns failed
		 *  when attempting dns failover) */
		ser_error=E_SEND;
		/* else return the last error (?) */
		/* the caller should take care and delete the transaction */
		return -1;
	}
	ser_error=0; /* clear branch send errors, we have overall success */
	set_kr(REQ_FWDED);
	return 1;
}



/* cancel handling/forwarding function
 * CANCELs with no matching transaction are handled in function of
 * the unmatched_cancel config var: they are either forwarded statefully,
 * statelessly or dropped.
 * function returns:
 *       1 - forward successful
 *       0 - error, but do not reply 
 *      <0 - error during forward
 * it also sets *tran if a transaction was created
 */
int t_forward_cancel(struct sip_msg* p_msg , struct proxy_l * proxy, int proto,
						struct cell** tran)
{
	struct cell* t_invite;
	struct cell* t;
	int ret;
	int new_tran;
	struct dest_info dst;
	str host;
	unsigned short port;
	short comp;
	
	t=0;
	/* handle cancels for which no transaction was created yet */
	if (cfg_get(tm, tm_cfg, unmatched_cancel)==UM_CANCEL_STATEFULL){
		/* create cancel transaction */
		new_tran=t_newtran(p_msg);
		if (new_tran<=0 && new_tran!=E_SCRIPT){
			if (new_tran==0)
				 /* retransmission => do nothing */
				ret=1;
			else
				/* some error => return it or DROP */
				ret=(ser_error==E_BAD_VIA && reply_to_via) ? 0: new_tran;
			goto end;
		}
		t=get_t();
		ret=t_forward_nonack(t, p_msg, proxy, proto);
		goto end;
	}
	
	t_invite=t_lookupOriginalT(  p_msg );
	if (t_invite!=T_NULL_CELL) {
		/* create cancel transaction */
		new_tran=t_newtran(p_msg);
		if (new_tran<=0 && new_tran!=E_SCRIPT){
			if (new_tran==0)
				 /* retransmission => do nothing */
				ret=1;
			else
				/* some error => return it or DROP */
				ret=(ser_error==E_BAD_VIA && reply_to_via) ? 0: new_tran;
			UNREF(t_invite);
			goto end;
		}
		t=get_t();
		e2e_cancel( p_msg, t, t_invite );
		UNREF(t_invite);
		ret=1;
		goto end;
	}else /* no coresponding INVITE transaction */
	     if (cfg_get(tm, tm_cfg, unmatched_cancel)==UM_CANCEL_DROP){
				DBG("t_forward_nonack: non matching cancel dropped\n");
				ret=1; /* do nothing -> drop */
				goto end;
		 }else{
			/* UM_CANCEL_STATELESS -> stateless forward */
				DBG( "SER: forwarding CANCEL statelessly \n");
				if (proxy==0) {
					init_dest_info(&dst);
					dst.proto=proto;
					if (get_uri_send_info(GET_NEXT_HOP(p_msg), &host,
								&port, &dst.proto, &comp)!=0){
						ret=E_BAD_ADDRESS;
						goto end;
					}
#ifdef USE_COMP
					dst.comp=comp;
#endif
					/* dst->send_sock not set, but forward_request 
					 * will take care of it */
					ret=forward_request(p_msg, &host, port, &dst);
					goto end;
				} else {
					init_dest_info(&dst);
					dst.proto=get_proto(proto, proxy->proto);
					proxy2su(&dst.to, proxy);
					/* dst->send_sock not set, but forward_request 
					 * will take care of it */
					ret=forward_request( p_msg , 0, 0, &dst) ;
					goto end;
				}
		}
end:
	if (tran)
		*tran=t;
	return ret;
}

/* Relays a CANCEL request if a corresponding INVITE transaction
 * can be found. The function is supposed to be used at the very
 * beginning of the script with reparse_invite=1 module parameter.
 *
 * return value:
 *    0: the CANCEL was successfully relayed
 *       (or error occured but reply cannot be sent) => DROP
 *    1: no corresponding INVITE transaction exisis
 *   <0: corresponding INVITE transaction exisis but error occured
 */
int t_relay_cancel(struct sip_msg* p_msg)
{
	struct cell* t_invite;
	struct cell* t;
	int ret;
	int new_tran;

	t_invite=t_lookupOriginalT(  p_msg );
	if (t_invite!=T_NULL_CELL) {
		/* create cancel transaction */
		new_tran=t_newtran(p_msg);
		if (new_tran<=0 && new_tran!=E_SCRIPT){
			if (new_tran==0)
				/* retransmission => DROP,
				t_newtran() takes care about it */
				ret=0;
			else
				/* some error => return it or DROP */
				ret=(ser_error==E_BAD_VIA && reply_to_via) ? 0: new_tran;
			UNREF(t_invite);
			goto end;
		}
		t=get_t();
		e2e_cancel( p_msg, t, t_invite );
		UNREF(t_invite);
		/* return 0 to stop the script processing */
		ret=0;
		goto end;

	} else {
		/* no corresponding INVITE trasaction found */
		ret=1;
	}
end:
	return ret;
}

/* WARNING: doesn't work from failure route (deadlock, uses t_relay_to which
 *  is failure route unsafe) */
int t_replicate(struct sip_msg *p_msg,  struct proxy_l *proxy, int proto )
{
	/* this is a quite horrible hack -- we just take the message
	   as is, including Route-s, Record-route-s, and Vias ,
	   forward it downstream and prevent replies received
	   from relaying by setting the replication/local_trans bit;

		nevertheless, it should be good enough for the primary
		customer of this function, REGISTER replication

		if we want later to make it thoroughly, we need to
		introduce delete lumps for all the header fields above
	*/
	return t_relay_to(p_msg, proxy, proto, 1 /* replicate */);
}

/* fixup function for reparse_on_dns_failover modparam */
int reparse_on_dns_failover_fixup(void *handle, str *name, void **val)
{
#ifdef USE_DNS_FAILOVER
	if ((int)(long)(*val) && mhomed) {
		LOG(L_WARN, "WARNING: reparse_on_dns_failover_fixup:"
		"reparse_on_dns_failover is enabled on a "
		"multihomed host -- check the readme of tm module!\n");
	}
#endif
	return 0;
}
