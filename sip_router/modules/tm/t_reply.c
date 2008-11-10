/*
 * $Id: t_reply.c,v 1.162 2008/11/10 12:47:02 tirpi Exp $
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
 *
 * History:
 * --------
 *  2003-01-19  faked lump list created in on_reply handlers
 *  2003-01-27  next baby-step to removing ZT - PRESERVE_ZT (jiri)
 *  2003-02-13  updated to use rb->dst (andrei)
 *  2003-02-18  replaced TOTAG_LEN w/ TOTAG_VALUE_LEN (TOTAG_LEN was defined
 *               twice with different values!)  (andrei)
 *  2003-02-28  scratchpad compatibility abandoned (jiri)
 *  2003-03-01  kr set through a function now (jiri)
 *  2003-03-06  saving of to-tags for ACK/200 matching introduced,
 *              voicemail changes accepted, updated to new callback
 *              names (jiri)
 *  2003-03-10  fixed new to tag bug/typo (if w/o {})  (andrei)
 *  2003-03-16  removed _TOTAG (jiri)
 *  2003-03-31  200 for INVITE/UAS resent even for UDP (jiri)
 *  2003-03-31  removed msg->repl_add_rm (andrei)
 *  2003-04-05  s/reply_route/failure_route, onreply_route introduced (jiri)
 *  2003-04-14  local acks generated before reply processing to avoid
 *              delays in length reply processing (like opening TCP
 *              connection to an unavailable destination) (jiri)
 *  2003-09-11  updates to new build_res_buf_from_sip_req() interface (bogdan)
 *  2003-09-11  t_reply_with_body() reshaped to use reply_lumps +
 *              build_res_buf_from_sip_req() instead of
 *              build_res_buf_with_body_from_sip_req() (bogdan)
 *  2003-11-05  flag context updated from failure/reply handlers back
 *              to transaction context (jiri)
 *  2003-11-11: build_lump_rpl() removed, add_lump_rpl() has flags (bogdan)
 *  2003-12-04  global TM callbacks switched to per transaction callbacks
 *              (bogdan)
 *  2004-02-06: support for user pref. added - destroy_avps (bogdan)
 *  2003-11-05  flag context updated from failure/reply handlers back
 *              to transaction context (jiri)
 *  2003-11-11: build_lump_rpl() removed, add_lump_rpl() has flags (bogdan)
 *  2004-02-13: t->is_invite and t->local replaced with flags (bogdan)
 *  2004-02-18  fifo_t_reply imported from vm module (bogdan)
 *  2004-08-23  avp list is available from failure/on_reply routes (bogdan)
 *  2004-10-01  added a new param.: restart_fr_on_each_reply (andrei)
 *  2005-03-01  force for statefull replies the incoming interface of
 *              the request (bogdan)
 *  2005-09-01  reverted to the old way of checking response.dst.send_sock
 *               in t_retransmit_reply & reply_light (andrei)
 *  2005-11-09  updated to the new timers interface (andrei)
 *  2006-02-07  named routes support (andrei)
 *  2006-09-13  t_pick_branch will skip also over branches with empty reply 
 *              t_should_relay_response will re-pick the branch if failure 
 *               route /handlers added new branches (andrei)
 * 2006-10-05  better final reply selection: t_pick_branch will prefer 6xx,
 *              if no 6xx reply => lowest class/code; if class==4xx =>
 *              prefer 401, 407, 415, 420 and 484   (andrei)
 * 2006-10-12  dns failover when a 503 is received
 *              replace a 503 final relayed reply by a 500 (andrei)
 * 2006-10-16  aggregate all the authorization headers/challenges when
 *               the final response is 401 or 407 (andrei)
 * 2007-03-08  membar_write() used in update_totag_set(...)(andrei)
 * 2007-03-15  build_local_ack: removed next_hop and replaced with dst to 
 *              avoid resolving next_hop twice
 *              added TMCB_ONSEND callbacks support for replies & ACKs (andrei)
 * 2007-05-28: build_ack() constructs the ACK from the
 *             outgoing INVITE instead of the incomming one.
 *             (it can be disabled with reparse_invite=0) (Miklos)
 * 2007-09-03: drop_replies() has been introduced (Miklos)
 * 2008-03-12  use cancel_b_method on 6xx (andrei)
 * 2008-05-30  make sure the wait timer is started after we don't need t
 *             anymore to allow safe calls from fr_timer (andrei)
 *
 */



#include "../../comp_defs.h"
#include "../../hash_func.h"
#include "../../dprint.h"
#include "../../config.h"
#include "../../parser/parser_f.h"
#include "../../ut.h"
#include "../../timer.h"
#include "../../error.h"
#include "../../action.h"
#include "../../dset.h"
#include "../../tags.h"
#include "../../data_lump.h"
#include "../../data_lump_rpl.h"
#include "../../usr_avp.h"
#include "../../atomic_ops.h" /* membar_write() */
#include "../../compiler_opt.h"
#include "../../select_buf.h" /* reset_static_buffer() */
#ifdef USE_DST_BLACKLIST
#include "../../dst_blacklist.h"
#endif
#ifdef USE_DNS_FAILOVER
#include "../../dns_cache.h"
#include "../../cfg_core.h" /* cfg_get(core, core_cfg, use_dns_failover) */
#endif

#include "defs.h"
#include "config.h"
#include "h_table.h"
#include "t_hooks.h"
#include "t_funcs.h"
#include "t_reply.h"
#include "t_cancel.h"
#include "t_msgbuilder.h"
#include "t_lookup.h"
#include "t_fwd.h"
#include "fix_lumps.h"
#include "t_stats.h"
#include "uac.h"


/* are we processing original or shmemed request ? */
enum route_mode rmode=MODE_REQUEST;

/* private place where we create to-tags for replies */
/* janakj: made public, I need to access this value to store it in dialogs */
char tm_tags[TOTAG_VALUE_LEN];
/* bogdan: pack tm_tag buffer and len into a str to pass them to
 * build_res_buf_from_sip_req() */
static str  tm_tag = {tm_tags,TOTAG_VALUE_LEN};
char *tm_tag_suffix;

/* where to go if there is no positive reply */
static int goto_on_negative=0;
/* where to go on receipt of reply */
static int goto_on_reply=0;
/* where to go on receipt of reply without transaction context */
int goto_on_sl_reply=0;


/* responses priority (used by t_pick_branch)
 *  0xx is used only for the initial value (=> should have no chance to be
 *  selected => the highest value); 1xx is not used */
static unsigned short resp_class_prio[]={
			32000, /* 0-99, special */
			11000, /* 1xx, special, should never be used */
				0,  /* 2xx, high priority (not used, 2xx are immediately 
				       forwarded and t_pick_branch will never be called if
					   a 2xx was received) */
			3000,  /* 3xx */
			4000,  /* 4xx */
			5000,  /* 5xx */
			1000   /* 6xx, highest priority */
};



/* we store the reply_route # in private memory which is
   then processed during t_relay; we cannot set this value
   before t_relay creates transaction context or after
   t_relay when a reply may arrive after we set this
   value; that's why we do it how we do it, i.e.,
   *inside*  t_relay using hints stored in private memory
   before t_relay is called
*/


void t_on_negative( unsigned int go_to )
{
	struct cell *t = get_t();

	/* in MODE_REPLY and MODE_ONFAILURE T will be set to current transaction;
	 * in MODE_REQUEST T will be set only if the transaction was already
	 * created; if not -> use the static variable */
	if (!t || t==T_UNDEFINED )
		goto_on_negative=go_to;
	else
		get_t()->on_negative = go_to;
}


void t_on_reply( unsigned int go_to )
{
	struct cell *t = get_t();

	/* in MODE_REPLY and MODE_ONFAILURE T will be set to current transaction;
	 * in MODE_REQUEST T will be set only if the transaction was already
	 * created; if not -> use the static variable */
	if (!t || t==T_UNDEFINED )
		goto_on_reply=go_to;
	else
		get_t()->on_reply = go_to;
}


unsigned int get_on_negative()
{
	return goto_on_negative;
}
unsigned int get_on_reply()
{
	return goto_on_reply;
}

void tm_init_tags()
{
	init_tags(tm_tags, &tm_tag_suffix,
		"SER-TM/tags", TM_TAG_SEPARATOR );
}

/* returns 0 if the message was previously acknowledged
 * (i.e., no E2EACK callback is needed) and one if the
 * callback shall be executed */
int unmatched_totag(struct cell *t, struct sip_msg *ack)
{
	struct totag_elem *i;
	str *tag;

	if (parse_headers(ack, HDR_TO_F,0)==-1 ||
				!ack->to ) {
		LOG(L_ERR, "ERROR: unmatched_totag: To invalid\n");
		return 1;
	}
	tag=&get_to(ack)->tag_value;
	i=t->fwded_totags;
	while(i){
		membar_depends(); /* make sure we don't see some old i content
							(needed on CPUs like Alpha) */
		if (i->tag.len==tag->len
				&& memcmp(i->tag.s, tag->s, tag->len)==0) {
			DBG("DEBUG: totag for e2e ACK found: %d\n", i->acked);
			/* mark totag as acked and return 0 if this was the first ack
			 * and 1 otherwise */
			return atomic_get_and_set_int(&i->acked, 1);
		}
		i=i->next;
	}
	/* surprising: to-tag never sighted before */
	return 1;
}

static inline void update_local_tags(struct cell *trans,
				struct bookmark *bm, char *dst_buffer,
				char *src_buffer /* to which bm refers */)
{
	if (bm->to_tag_val.s) {
		trans->uas.local_totag.s=bm->to_tag_val.s-src_buffer+dst_buffer;
		trans->uas.local_totag.len=bm->to_tag_val.len;
	}
}


/* append a newly received tag from a 200/INVITE to
 * transaction's set; (only safe if called from within
 * a REPLY_LOCK); it returns 1 if such a to tag already
 * exists
 */
inline static int update_totag_set(struct cell *t, struct sip_msg *ok)
{
	struct totag_elem *i, *n;
	str *tag;
	char *s;

	if (!ok->to || !ok->to->parsed) {
		LOG(L_ERR, "ERROR: update_totag_set: to not parsed\n");
		return 0;
	}
	tag=&get_to(ok)->tag_value;
	if (!tag->s) {
		DBG("ERROR: update_totag_set: no tag in to\n");
		return 0;
	}

	for (i=t->fwded_totags; i; i=i->next) {
		if (i->tag.len==tag->len
				&& memcmp(i->tag.s, tag->s, tag->len) ==0 ){
			/* to tag already recorded */
#ifdef XL_DEBUG
			LOG(L_CRIT, "DEBUG: update_totag_set: totag retransmission\n");
#else
			DBG("DEBUG: update_totag_set: totag retransmission\n");
#endif
			return 1;
		}
	}
	/* that's a new to-tag -- record it */
	shm_lock();
	n=(struct totag_elem*) shm_malloc_unsafe(sizeof(struct totag_elem));
	s=(char *)shm_malloc_unsafe(tag->len);
	shm_unlock();
	if (!s || !n) {
		LOG(L_ERR, "ERROR: update_totag_set: no  memory \n");
		if (n) shm_free(n);
		if (s) shm_free(s);
		return 0;
	}
	memset(n, 0, sizeof(struct totag_elem));
	memcpy(s, tag->s, tag->len );
	n->tag.s=s;n->tag.len=tag->len;
	n->next=t->fwded_totags;
	membar_write(); /* make sure all the changes to n are visible on all cpus
					   before we update t->fwded_totags. This is needed for
					   three reasons: the compiler might reorder some of the 
					   writes, the cpu/cache could also reorder them with
					   respect to the visibility on other cpus
					   (e.g. some of the changes to n could be visible on
					    another cpu _after_ seeing t->fwded_totags=n) and
					   the "readers" (unmatched_tags()) do not use locks and
					   can be called simultaneously on another cpu.*/
	t->fwded_totags=n;
	DBG("DEBUG: update_totag_set: new totag \n");
	return 0;
}


/*
 * Build an ACK to a negative reply
 */
static char *build_ack(struct sip_msg* rpl,struct cell *trans,int branch,
	unsigned int *ret_len)
{
	str to;

    if (parse_headers(rpl,HDR_TO_F, 0)==-1 || !rpl->to ) {
        LOG(L_ERR, "ERROR: build_ack: "
            "cannot generate a HBH ACK if key HFs in reply missing\n");
        return NULL;
    }
	to.s=rpl->to->name.s;
	to.len=rpl->to->len;

	if (cfg_get(tm, tm_cfg, reparse_invite)) {
		/* build the ACK from the INVITE which was sent out */
		return build_local_reparse( trans, branch, ret_len,
					ACK, ACK_LEN, &to );
	} else {
		/* build the ACK from the reveived INVITE */
		return build_local( trans, branch, ret_len,
					ACK, ACK_LEN, &to );
	}
}


/*
 * The function builds an ACK to 200 OK of local transactions, honoring the
 * route set.
 * The destination to which the message should be sent will be returned
 * in the dst parameter.
 * returns 0 on error and a pkg_malloc'ed buffer with length in ret_len
 *  and intended destination in dst on success.
 */
static char *build_local_ack(struct sip_msg* rpl, struct cell *trans, 
								int branch, unsigned int *ret_len,
								struct dest_info*  dst)
{
	str to;
	if (parse_headers(rpl, HDR_EOH_F, 0) == -1 || !rpl->to) {
		LOG(L_ERR, "ERROR: build_local_ack: Error while parsing headers\n");
		return 0;
	}

	to.s = rpl->to->name.s;
	to.len = rpl->to->len;
	return build_dlg_ack(rpl, trans, branch, &to, ret_len, dst);
}



#if 0 /* candidate for removal --andrei */
     /*
      * The function is used to send a localy generated ACK to INVITE
      * (tm generates the ACK on behalf of application using UAC
      */
static int send_local_ack(struct sip_msg* msg, str* next_hop,
							char* ack, int ack_len)
{
	struct dest_info dst;
#ifdef USE_DNS_FAILOVER
	struct dns_srv_handle dns_h;
#endif

	if (!next_hop) {
		LOG(L_ERR, "send_local_ack: Invalid parameter value\n");
		return -1;
	}
#ifdef USE_DNS_FAILOVER
	if (cfg_get(core, core_cfg, use_dns_failover)){
		dns_srv_handle_init(&dns_h);
		if ((uri2dst(&dns_h, &dst, msg,  next_hop, PROTO_NONE)==0) || 
				(dst.send_sock==0)){
			dns_srv_handle_put(&dns_h);
			LOG(L_ERR, "send_local_ack: no socket found\n");
			return -1;
		}
		dns_srv_handle_put(&dns_h); /* not needed anymore */
	}else{
		if ((uri2dst(0, &dst, msg,  next_hop, PROTO_NONE)==0) || 
				(dst.send_sock==0)){
			LOG(L_ERR, "send_local_ack: no socket found\n");
			return -1;
		}
	}
#else
	if ((uri2dst(&dst, msg,  next_hop, PROTO_NONE)==0) || (dst.send_sock==0)){
		LOG(L_ERR, "send_local_ack: no socket found\n");
		return -1;
	}
#endif
	return msg_send(&dst, ack, ack_len);
}
#endif



inline static void start_final_repl_retr( struct cell *t )
{
	if (unlikely(!is_local(t) && t->uas.request->REQ_METHOD==METHOD_INVITE )){
		/* crank timers for negative replies */
		if (t->uas.status>=300) {
			if (start_retr(&t->uas.response)!=0)
				LOG(L_CRIT, "BUG: start_final_repl_retr: start retr failed"
						" for %p\n", &t->uas.response);
			return;
		}
		/* local UAS retransmits too */
		if (t->relayed_reply_branch==-2 && t->uas.status>=200) {
			/* we retransmit 200/INVs regardless of transport --
			   even if TCP used, UDP could be used upstream and
			   loose the 200, which is not retransmitted by proxies
			*/
			if (force_retr( &t->uas.response )!=0)
				LOG(L_CRIT, "BUG: start_final_repl_retr: force retr failed for"
						" %p\n", &t->uas.response);
			return;
		}
	}
}



static int _reply_light( struct cell *trans, char* buf, unsigned int len,
			 unsigned int code, char * text,
			 char *to_tag, unsigned int to_tag_len, int lock,
			 struct bookmark *bm	)
{
	struct retr_buf *rb;
	unsigned int buf_len;
	branch_bm_t cancel_bitmap;
#ifdef TMCB_ONSEND
	struct tmcb_params onsend_params;
#endif

	if (!buf)
	{
		DBG("DEBUG: _reply_light: response building failed\n");
		/* determine if there are some branches to be canceled */
		if ( is_invite(trans) ) {
			if (lock) LOCK_REPLIES( trans );
			which_cancel(trans, &cancel_bitmap );
			if (lock) UNLOCK_REPLIES( trans );
		}
		/* and clean-up, including cancellations, if needed */
		goto error;
	}

	cancel_bitmap=0;
	if (lock) LOCK_REPLIES( trans );
	if ( is_invite(trans) ) which_cancel(trans, &cancel_bitmap );
	if (trans->uas.status>=200) {
		LOG( L_ERR, "ERROR: _reply_light: can't generate %d reply"
			" when a final %d was sent out\n", code, trans->uas.status);
		goto error2;
	}

	rb = & trans->uas.response;
	rb->activ_type=code;

	trans->uas.status = code;
	buf_len = rb->buffer ? len : len + REPLY_OVERBUFFER_LEN;
	rb->buffer = (char*)shm_resize( rb->buffer, buf_len );
	/* puts the reply's buffer to uas.response */
	if (! rb->buffer ) {
			LOG(L_ERR, "ERROR: _reply_light: cannot allocate shmem buffer\n");
			goto error3;
	}
	update_local_tags(trans, bm, rb->buffer, buf);

	rb->buffer_len = len ;
	memcpy( rb->buffer , buf , len );
	/* needs to be protected too because what timers are set depends
	   on current transactions status */
	/* t_update_timers_after_sending_reply( rb ); */
	update_reply_stats( code );
	trans->relayed_reply_branch=-2;
	t_stats_replied_locally();
	if (lock) UNLOCK_REPLIES( trans );

	/* do UAC cleanup procedures in case we generated
	   a final answer whereas there are pending UACs */
	if (code>=200) {
		if ( is_local(trans) ) {
			DBG("DEBUG: local transaction completed from _reply\n");
			if ( unlikely(has_tran_tmcbs(trans, TMCB_LOCAL_COMPLETED)) )
				run_trans_callbacks( TMCB_LOCAL_COMPLETED, trans,
					0, FAKED_REPLY, code);
		} else {
			if ( unlikely(has_tran_tmcbs(trans, TMCB_RESPONSE_OUT)) )
				run_trans_callbacks( TMCB_RESPONSE_OUT, trans,
					trans->uas.request, FAKED_REPLY, code);
		}

		cleanup_uac_timers( trans );
		if (is_invite(trans)) 
			cancel_uacs( trans, cancel_bitmap, F_CANCEL_B_KILL );
		start_final_repl_retr(  trans );
	}

	/* send it out */
	/* first check if we managed to resolve topmost Via -- if
	   not yet, don't try to retransmit
	*/
	/*
	   response.dst.send_sock might be unset if the process that created
	   the original transaction has not finished initialising the
	   retransmission buffer (see t_newtran/ init_rb).
	   If reply_to_via is set and via contains a host name (and not an ip)
	   the chances for this increase a lot.
	 */
	if (!trans->uas.response.dst.send_sock) {
		LOG(L_ERR, "ERROR: _reply_light: no resolved dst to send reply to\n");
	} else {
#ifdef TMCB_ONSEND
		if (SEND_PR_BUFFER( rb, buf, len )>=0)
			if (unlikely(has_tran_tmcbs(trans, TMCB_RESPONSE_SENT))){
				INIT_TMCB_ONSEND_PARAMS(onsend_params, 0, 0, rb, &rb->dst, 
								buf, len, TMCB_LOCAL_F, rb->branch, code);
				run_onsend_callbacks2(TMCB_RESPONSE_SENT, trans,
										&onsend_params);
			}
#else
		SEND_PR_BUFFER( rb, buf, len );
#endif
		DBG("DEBUG: reply sent out. buf=%p: %.9s..., shmem=%p: %.9s\n",
			buf, buf, rb->buffer, rb->buffer );
	}
	if (code>=200) {
		/* start wait timer after finishing with t so that this function can
		 * be safely called from a fr_timer which allows quick timer dels
		 * (timer_allow_del()) (there's no chance of having the wait handler
		 *  executed while we still need t) --andrei */
		put_on_wait(trans);
	}
	pkg_free( buf ) ;
	DBG("DEBUG: _reply_light: finished\n");
	return 1;

error3:
error2:
	if (lock) UNLOCK_REPLIES( trans );
	pkg_free ( buf );
error:
	/* do UAC cleanup */
	cleanup_uac_timers( trans );
	if ( is_invite(trans) )
		cancel_uacs( trans, cancel_bitmap, F_CANCEL_B_KILL);
	/* we did not succeed -- put the transaction on wait */
	put_on_wait(trans);
	return -1;
}


/* send a UAS reply
 * returns 1 if everything was OK or -1 for error
 */
static int _reply( struct cell *trans, struct sip_msg* p_msg,
	unsigned int code, char * text, int lock )
{
	unsigned int len;
	char * buf, *dset;
	struct bookmark bm;
	int dset_len;

	if (code>=200) set_kr(REQ_RPLD);
	/* compute the buffer in private memory prior to entering lock;
	 * create to-tag if needed */

	/* if that is a redirection message, dump current message set to it */
	if (code>=300 && code<400) {
		dset=print_dset(p_msg, &dset_len);
		if (dset) {
			add_lump_rpl(p_msg, dset, dset_len, LUMP_RPL_HDR);
		}
	}

	if (code>=180 && p_msg->to
				&& (get_to(p_msg)->tag_value.s==0
			    || get_to(p_msg)->tag_value.len==0)) {
		calc_crc_suffix( p_msg, tm_tag_suffix );
		buf = build_res_buf_from_sip_req(code,text, &tm_tag, p_msg, &len, &bm);
		return _reply_light( trans, buf, len, code, text,
			tm_tag.s, TOTAG_VALUE_LEN, lock, &bm);
	} else {
		buf = build_res_buf_from_sip_req(code,text, 0 /*no to-tag*/,
			p_msg, &len, &bm);

		return _reply_light(trans,buf,len,code,text,
			0, 0, /* no to-tag */lock, &bm);
	}
}


/*if msg is set -> it will fake the env. vars conforming with the msg; if NULL
 * the env. will be restore to original */
void faked_env( struct cell *t,struct sip_msg *msg)
{
	static enum route_mode backup_mode;
	static struct cell *backup_t;
	static unsigned int backup_msgid;
	static avp_list_t* backup_user_from, *backup_user_to;
	static avp_list_t* backup_domain_from, *backup_domain_to;
	static avp_list_t* backup_uri_from, *backup_uri_to;
	static struct socket_info* backup_si;

	if (msg) {
		/* remember we are back in request processing, but process
		 * a shmem-ed replica of the request; advertise it in rmode;
		 * for example t_reply needs to know that
		 */
		backup_mode=rmode;
		rmode=MODE_ONFAILURE;
		/* also, tm actions look in beginning whether transaction is
		 * set -- whether we are called from a reply-processing
		 * or a timer process, we need to set current transaction;
		 * otherwise the actions would attempt to look the transaction
		 * up (unnecessary overhead, refcounting)
		 */
		/* backup */
		backup_t=get_t();
		backup_msgid=global_msg_id;
		/* fake transaction and message id */
		global_msg_id=msg->id;
		set_t(t);
		/* make available the avp list from transaction */

		backup_uri_from = set_avp_list(AVP_TRACK_FROM | AVP_CLASS_URI, &t->uri_avps_from );
		backup_uri_to = set_avp_list(AVP_TRACK_TO | AVP_CLASS_URI, &t->uri_avps_to );
		backup_user_from = set_avp_list(AVP_TRACK_FROM | AVP_CLASS_USER, &t->user_avps_from );
		backup_user_to = set_avp_list(AVP_TRACK_TO | AVP_CLASS_USER, &t->user_avps_to );
		backup_domain_from = set_avp_list(AVP_TRACK_FROM | AVP_CLASS_DOMAIN, &t->domain_avps_from );
		backup_domain_to = set_avp_list(AVP_TRACK_TO | AVP_CLASS_DOMAIN, &t->domain_avps_to );
		/* set default send address to the saved value */
		backup_si=bind_address;
		bind_address=t->uac[0].request.dst.send_sock;
	} else {
		/* restore original environment */
		set_t(backup_t);
		global_msg_id=backup_msgid;
		rmode=backup_mode;
		/* restore original avp list */
		set_avp_list(AVP_TRACK_FROM | AVP_CLASS_USER, backup_user_from );
		set_avp_list(AVP_TRACK_TO | AVP_CLASS_USER, backup_user_to );
		set_avp_list(AVP_TRACK_FROM | AVP_CLASS_DOMAIN, backup_domain_from );
		set_avp_list(AVP_TRACK_TO | AVP_CLASS_DOMAIN, backup_domain_to );
		set_avp_list(AVP_TRACK_FROM | AVP_CLASS_URI, backup_uri_from );
		set_avp_list(AVP_TRACK_TO | AVP_CLASS_URI, backup_uri_to );
		bind_address=backup_si;
	}
}


int fake_req(struct sip_msg *faked_req,
							struct sip_msg *shmem_msg, int extra_flags)
{
	/* on_negative_reply faked msg now copied from shmem msg (as opposed
	 * to zero-ing) -- more "read-only" actions (exec in particular) will
	 * work from reply_route as they will see msg->from, etc.; caution,
	 * rw actions may append some pkg stuff to msg, which will possibly be
	 * never released (shmem is released in a single block) */
	memcpy( faked_req, shmem_msg, sizeof(struct sip_msg));

	/* if we set msg_id to something different from current's message
	 * id, the first t_fork will properly clean new branch URIs */
	faked_req->id=shmem_msg->id-1;
	/* msg->parsed_uri_ok must be reset since msg_parsed_uri is
	 * not cloned (and cannot be cloned) */
	faked_req->parsed_uri_ok = 0;
	
	faked_req->msg_flags|=extra_flags; /* set the extra tm flags */
	/* new_uri can change -- make a private copy */
	if (shmem_msg->new_uri.s!=0 && shmem_msg->new_uri.len!=0) {
		faked_req->new_uri.s=pkg_malloc(shmem_msg->new_uri.len+1);
		if (!faked_req->new_uri.s) {
			LOG(L_ERR, "ERROR: fake_req: no uri/pkg mem\n");
			goto error00;
		}
		faked_req->new_uri.len=shmem_msg->new_uri.len;
		memcpy( faked_req->new_uri.s, shmem_msg->new_uri.s,
			faked_req->new_uri.len);
		faked_req->new_uri.s[faked_req->new_uri.len]=0;
	}
	/* dst_uri can change ALSO!!! -- make a private copy */
	if (shmem_msg->dst_uri.s!=0 && shmem_msg->dst_uri.len!=0) {
		faked_req->dst_uri.s=pkg_malloc(shmem_msg->dst_uri.len+1);
		if (!faked_req->dst_uri.s) {
			LOG(L_ERR, "ERROR: fake_req: no uri/pkg mem\n");
			goto error00;
		}
		faked_req->dst_uri.len=shmem_msg->dst_uri.len;
		memcpy( faked_req->dst_uri.s, shmem_msg->dst_uri.s,
			faked_req->dst_uri.len);
		faked_req->dst_uri.s[faked_req->dst_uri.len]=0;
	}

	return 1;
error00:
	return 0;
}

void free_faked_req(struct sip_msg *faked_req, struct cell *t)
{
	struct hdr_field *hdr;

	if (faked_req->new_uri.s) {
		pkg_free(faked_req->new_uri.s);
		faked_req->new_uri.s = 0;
	}

	if (faked_req->dst_uri.s) {
		pkg_free(faked_req->dst_uri.s);
		faked_req->dst_uri.s = 0;
	}

	/* free all types of lump that were added in failure handlers */
	del_nonshm_lump( &(faked_req->add_rm) );
	del_nonshm_lump( &(faked_req->body_lumps) );
	del_nonshm_lump_rpl( &(faked_req->reply_lump) );

	/* free header's parsed structures that were added by failure handlers */
	for( hdr=faked_req->headers ; hdr ; hdr=hdr->next ) {
		if ( hdr->parsed && hdr_allocs_parse(hdr) &&
		(hdr->parsed<(void*)t->uas.request ||
		hdr->parsed>=(void*)t->uas.end_request)) {
			/* header parsed filed doesn't point inside uas.request memory
			 * chunck -> it was added by failure funcs.-> free it as pkg */
			DBG("DBG:free_faked_req: removing hdr->parsed %d\n",
					hdr->type);
			clean_hdr_field(hdr);
			hdr->parsed = 0;
		}
	}
}


/* return 1 if a failure_route processes */
int run_failure_handlers(struct cell *t, struct sip_msg *rpl,
					int code, int extra_flags)
{
	static struct sip_msg faked_req;
	struct sip_msg *shmem_msg = t->uas.request;
	int on_failure;
	struct run_act_ctx ra_ctx;

	/* failure_route for a local UAC? */
	if (!shmem_msg) {
		LOG(L_WARN,"Warning: run_failure_handlers: no UAC support (%d, %d) \n",
			t->on_negative, t->tmcb_hl.reg_types);
		return 0;
	}

	/* don't start faking anything if we don't have to */
	if (unlikely(!t->on_negative && !has_tran_tmcbs( t, TMCB_ON_FAILURE))) {
		LOG(L_WARN,
			"Warning: run_failure_handlers: no negative handler (%d, %d)\n",
			t->on_negative,
			t->tmcb_hl.reg_types);
		return 1;
	}

	if (!fake_req(&faked_req, shmem_msg, extra_flags)) {
		LOG(L_ERR, "ERROR: run_failure_handlers: fake_req failed\n");
		return 0;
	}
	/* fake also the env. conforming to the fake msg */
	faked_env( t, &faked_req);
	/* DONE with faking ;-) -> run the failure handlers */

	if (unlikely(has_tran_tmcbs( t, TMCB_ON_FAILURE)) ) {
		run_trans_callbacks( TMCB_ON_FAILURE, t, &faked_req, rpl, code);
	}
	if (t->on_negative) {
		/* avoid recursion -- if failure_route forwards, and does not
		 * set next failure route, failure_route will not be reentered
		 * on failure */
		on_failure = t->on_negative;
		t->on_negative=0;
		reset_static_buffer();
		/* run a reply_route action if some was marked */
		init_run_actions_ctx(&ra_ctx);
		if (run_actions(&ra_ctx, failure_rt.rlist[on_failure], &faked_req)<0)
			LOG(L_ERR, "ERROR: run_failure_handlers: Error in do_action\n");
	}

	/* restore original environment and free the fake msg */
	faked_env( t, 0);
	free_faked_req(&faked_req,t);

	/* if failure handler changed flag, update transaction context */
	shmem_msg->flags = faked_req.flags;
	return 1;
}



/* 401, 407, 415, 420, and 484 have priority over the other 4xx*/
inline static short int get_4xx_prio(unsigned char xx)
{
	switch(xx){
		case  1:
		case  7:
		case 15:
		case 20:
		case 84:
			return xx;
			break;
	}
	return 100+xx;
}



/* returns response priority, lower number => highest prio
 *
 * responses                    priority val
 *  0-99                        32000+reponse         (special)
 *  1xx                         11000+reponse         (special)
 *  700-999                     10000+response        (very low)
 *  5xx                          5000+xx              (low)
 *  4xx                          4000+xx
 *  3xx                          3000+xx
 *  6xx                          1000+xx              (high)
 *  2xx                          0000+xx              (highest) 
 */
inline static short int get_prio(unsigned int resp)
{
	int class;
	int xx;
	
	class=resp/100;

	if (class<7){
		xx=resp%100;
		return resp_class_prio[class]+((class==4)?get_4xx_prio(xx):xx);
	}
	return 10000+resp; /* unknown response class => return very low prio */
}



/* select a branch for forwarding; returns:
 * 0..X ... branch number
 * -1   ... error
 * -2   ... can't decide yet -- incomplete branches present
 */
int t_pick_branch(int inc_branch, int inc_code, struct cell *t, int *res_code)
{
	int best_b, best_s, b;

	best_b=-1; best_s=0;
	for ( b=0; b<t->nr_of_outgoings ; b++ ) {
		/* "fake" for the currently processed branch */
		if (b==inc_branch) {
			if (get_prio(inc_code)<get_prio(best_s)) {
				best_b=b;
				best_s=inc_code;
			}
			continue;
		}
		/* skip 'empty branches' */
		if (!t->uac[b].request.buffer) continue;
		/* there is still an unfinished UAC transaction; wait now! */
		if ( t->uac[b].last_received<200 )
			return -2;
		/* if reply is null => t_send_branch "faked" reply, skip over it */
		if ( t->uac[b].reply && 
				get_prio(t->uac[b].last_received)<get_prio(best_s) ) {
			best_b =b;
			best_s = t->uac[b].last_received;
		}
	} /* find lowest branch */
	
	*res_code=best_s;
	return best_b;
}


/* flag indicating whether it is requested
 * to drop the already saved replies or not */
static unsigned char drop_replies;

/* This is the neurological point of reply processing -- called
 * from within a REPLY_LOCK, t_should_relay_response decides
 * how a reply shall be processed and how transaction state is
 * affected.
 *
 * Checks if the new reply (with new_code status) should be sent or not
 *  based on the current
 * transaction status.
 * Returns 	- branch number (0,1,...) which should be relayed
 *         -1 if nothing to be relayed
 */
static enum rps t_should_relay_response( struct cell *Trans , int new_code,
	int branch , int *should_store, int *should_relay,
	branch_bm_t *cancel_bitmap, struct sip_msg *reply )
{
	int branch_cnt;
	int picked_branch;
	int picked_code;
	int new_branch;
	int inv_through;
	int extra_flags;
	int i;

	/* note: this code never lets replies to CANCEL go through;
	   we generate always a local 200 for CANCEL; 200s are
	   not relayed because it's not an INVITE transaction;
	   >= 300 are not relayed because 200 was already sent
	   out
	*/
	DBG("->>>>>>>>> T_code=%d, new_code=%d\n",Trans->uas.status,new_code);
	inv_through=new_code>=200 && new_code<300 && is_invite(Trans);
	/* if final response sent out, allow only INVITE 2xx  */
	if ( Trans->uas.status >= 200 ) {
		if (inv_through) {
			DBG("DBG: t_should_relay_response: 200 INV after final sent\n");
			*should_store=0;
			Trans->uac[branch].last_received=new_code;
			*should_relay=branch;
			return RPS_PUSHED_AFTER_COMPLETION;
		}
		/* except the exception above, too late  messages will
		   be discarded */
		goto discard;
	}

	/* if final response received at this branch, allow only INVITE 2xx */
	if (Trans->uac[branch].last_received>=200
			&& !(inv_through && Trans->uac[branch].last_received<300)) {
		/* don't report on retransmissions */
		if (Trans->uac[branch].last_received==new_code) {
			DBG("DEBUG: final reply retransmission\n");
			goto discard;
		}
		/* if you FR-timed-out, faked a local 408  and 487 came or
		 *  faked a CANCEL on a non-replied branch don't
		 * report on it either */
		if ((Trans->uac[branch].last_received==487) || 
				(Trans->uac[branch].last_received==408 && new_code==487)) {
			DBG("DEBUG: %d came for a %d branch (ignored)\n",
					new_code, Trans->uac[branch].last_received);
			goto discard;
		}
		/* this looks however how a very strange status rewrite attempt;
		 * report on it */
		LOG(L_ERR, "ERROR: t_should_relay_response: status rewrite by UAS: "
			"stored: %d, received: %d\n",
			Trans->uac[branch].last_received, new_code );
		goto discard;
	}


	/* no final response sent yet */
	/* negative replies subject to fork picking */
	if (new_code >=300 ) {

		Trans->uac[branch].last_received=new_code;

		/* if all_final return lowest */
		picked_branch=t_pick_branch(branch,new_code, Trans, &picked_code);
		if (picked_branch==-2) { /* branches open yet */
			*should_store=1;
			*should_relay=-1;
			if (new_code>=600 && new_code<=699){
				if (!(Trans->flags & T_6xx)){
					/* cancel only the first time we get a 6xx */
					which_cancel(Trans, cancel_bitmap);
					Trans->flags|=T_6xx;
				}
			}
			return RPS_STORE;
		}
		if (picked_branch==-1) {
			LOG(L_CRIT, "ERROR: t_should_relay_response: lowest==-1\n");
			goto error;
		}

		/* no more pending branches -- try if that changes after
		   a callback; save branch count to be able to determine
		   later if new branches were initiated */
		branch_cnt=Trans->nr_of_outgoings;
		/* also append the current reply to the transaction to
		 * make it available in failure routes - a kind of "fake"
		 * save of the final reply per branch */
		Trans->uac[branch].reply = reply;
		Trans->flags&=~T_6xx; /* clear the 6xx flag , we want to 
								 allow new branches from the failure route */

		drop_replies = 0;
		/* run ON_FAILURE handlers ( route and callbacks) */
		if (unlikely(has_tran_tmcbs( Trans, TMCB_ON_FAILURE_RO|TMCB_ON_FAILURE)
						|| Trans->on_negative )) {
			extra_flags=
				((Trans->uac[picked_branch].request.flags & F_RB_TIMEOUT)?
							FL_TIMEOUT:0) | 
				((Trans->uac[picked_branch].request.flags & F_RB_REPLIED)?
						 	FL_REPLIED:0);
			run_failure_handlers( Trans, Trans->uac[picked_branch].reply,
									picked_code, extra_flags);
			if (unlikely(drop_replies)) {
				/* drop all the replies that we have already saved */
				for (i=0; i<branch_cnt; i++) {
					if (Trans->uac[i].reply &&
					(Trans->uac[i].reply != FAKED_REPLY) &&
					(Trans->uac[i].reply->msg_flags & FL_SHM_CLONE))
						/* we have to drop the reply which is already in shm mem */
						sip_msg_free(Trans->uac[i].reply);

					Trans->uac[i].reply = 0;
				}
				/* make sure that the selected reply is not relayed even if
				there is not any new branch added -- should not happen */
				picked_branch = -1;
			}
		}

		/* now reset it; after the failure logic, the reply may
		 * not be stored any more and we don't want to keep into
		 * transaction some broken reference */
		Trans->uac[branch].reply = 0;

		/* look if the callback perhaps replied transaction; it also
		   covers the case in which a transaction is replied localy
		   on CANCEL -- then it would make no sense to proceed to
		   new branches bellow
		*/
		if (Trans->uas.status >= 200) {
			*should_store=0;
			*should_relay=-1;
			/* this might deserve an improvement -- if something
			   was already replied, it was put on wait and then,
			   returning RPS_COMPLETED will make t_on_reply
			   put it on wait again; perhaps splitting put_on_wait
			   from send_reply or a new RPS_ code would be healthy
			*/
			return RPS_COMPLETED;
		}
		/* look if the callback/failure_route introduced new branches ... */
		if (branch_cnt<Trans->nr_of_outgoings){
			/* the new branches might be already "finished" => we
			 * must use t_pick_branch again */
			new_branch=t_pick_branch((drop_replies==0)?
							branch :
							-1, /* make sure we do not pick
								the current branch */
						new_code,
						Trans,
						&picked_code);

			if (new_branch<0){
				if (likely(drop_replies==0)) {
					if (new_branch==-2) { /* branches open yet */
						*should_store=1;
						*should_relay=-1;
						return RPS_STORE;
					}
					/* error, use the old picked_branch */
				} else {
					if (new_branch==-2) { /* branches open yet */
						/* we are not allowed to relay the reply */
						*should_store=0;
						*should_relay=-1;
						return RPS_DISCARDED;
					} else {
						/* There are no open branches,
						and all the newly created branches failed
						as well. We are not allowed to send back
						the previously picked-up branch, thus,
						let us reply with an error instead. */
						goto branches_failed;
					}
				}
			}else{
				/* found a new_branch */
				picked_branch=new_branch;
			}
		} else if (unlikely(drop_replies)) {
			/* Either the script writer did not add new branches
			after calling t_drop_replies(), or tm was unable
			to add the new branches to the transaction. */
			goto branches_failed;
		}

		/* really no more pending branches -- return lowest code */
		*should_store=0;
		*should_relay=picked_branch;
		/* we dont need 'which_cancel' here -- all branches
		   known to have completed */
		/* which_cancel( Trans, cancel_bitmap ); */
		return RPS_COMPLETED;
	}

	/* not >=300 ... it must be 2xx or provisional 1xx */
	if (new_code>=100) {
		/* 1xx and 2xx except 100 will be relayed */
		Trans->uac[branch].last_received=new_code;
		*should_store=0;
		*should_relay= new_code==100? -1 : branch;
		if (new_code>=200 ) {
			which_cancel( Trans, cancel_bitmap );
			return RPS_COMPLETED;
		} else return RPS_PROVISIONAL;
	}

error:
	/* reply_status didn't match -- it must be something weird */
	LOG(L_CRIT, "ERROR: Oh my gooosh! We don't know whether to relay %d\n",
		new_code);
discard:
	*should_store=0;
	*should_relay=-1;
	return RPS_DISCARDED;

branches_failed:
	*should_store=0;
	*should_relay=-1;
	/* We have hopefully set tm_error in failure_route when
	the branches failed. If not, reply with E_UNSPEC */
	if ((kill_transaction_unsafe(Trans,
				    tm_error ? tm_error : E_UNSPEC)
				    ) <=0 ){
		LOG(L_ERR, "ERROR: t_should_relay_response: "
			"reply generation failed\n");
	}
	
	return RPS_COMPLETED;
}

/* Retransmits the last sent inbound reply.
 * input: p_msg==request for which I want to retransmit an associated reply
 * Returns  -1 - error
 *           1 - OK
 */
int t_retransmit_reply( struct cell *t )
{
	static char b[BUF_SIZE];
	int len;

	/* first check if we managed to resolve topmost Via -- if
	   not yet, don't try to retransmit
	*/
	/*
	   response.dst.send_sock might be unset if the process that created
	   the original transaction has not finished initialising the
	   retransmission buffer (see t_newtran/ init_rb).
	   If reply_to_via is set and via contains a host name (and not an ip)
	   the chances for this increase a lot.
	 */
	if (!t->uas.response.dst.send_sock) {
		LOG(L_WARN, "WARNING: t_retransmit_reply: "
			"no resolved dst to retransmit\n");
		return -1;
	}

	/* we need to lock the transaction as messages from
	   upstream may change it continuously
	*/
	LOCK_REPLIES( t );

	if (!t->uas.response.buffer) {
		DBG("DBG: t_retransmit_reply: nothing to retransmit\n");
		goto error;
	}

	len=t->uas.response.buffer_len;
	if ( len==0 || len>BUF_SIZE )  {
		DBG("DBG: t_retransmit_reply: "
			"zero length or too big to retransmit: %d\n", len);
		goto error;
	}
	memcpy( b, t->uas.response.buffer, len );
	UNLOCK_REPLIES( t );
	SEND_PR_BUFFER( & t->uas.response, b, len );
#ifdef TMCB_ONSEND
	if (unlikely(has_tran_tmcbs(t, TMCB_RESPONSE_SENT))){ 
		/* we don't know if it's a retransmission of a local reply or a 
		 * forwarded reply */
		run_onsend_callbacks(TMCB_RESPONSE_SENT, &t->uas.response, 0, 0,
								TMCB_RETR_F);
	}
#endif
	DBG("DEBUG: reply retransmitted. buf=%p: %.9s..., shmem=%p: %.9s\n",
		b, b, t->uas.response.buffer, t->uas.response.buffer );
	return 1;

error:
	UNLOCK_REPLIES(t);
	return -1;
}




int t_reply( struct cell *t, struct sip_msg* p_msg, unsigned int code,
	char * text )
{
	return _reply( t, p_msg, code, text, 1 /* lock replies */ );
}

int t_reply_unsafe( struct cell *t, struct sip_msg* p_msg, unsigned int code,
	char * text )
{
	return _reply( t, p_msg, code, text, 0 /* don't lock replies */ );
}



void set_final_timer( struct cell *t )
{
	start_final_repl_retr(t);
	put_on_wait(t);
}


void cleanup_uac_timers( struct cell *t )
{
	int i;

	/* reset FR/retransmission timers */
	for (i=0; i<t->nr_of_outgoings; i++ ){
		stop_rb_timers(&t->uac[i].request);
	}
	DBG("DEBUG: cleanup_uac_timers: RETR/FR timers reset\n");
}

static int store_reply( struct cell *trans, int branch, struct sip_msg *rpl)
{
#		ifdef EXTRA_DEBUG
		if (trans->uac[branch].reply) {
			LOG(L_ERR, "ERROR: replacing stored reply; aborting\n");
			abort();
		}
#		endif

		/* when we later do things such as challenge aggregation,
	   	   we should parse the message here before we conserve
		   it in shared memory; -jiri
		*/
		if (rpl==FAKED_REPLY)
			trans->uac[branch].reply=FAKED_REPLY;
		else
			trans->uac[branch].reply = sip_msg_cloner( rpl, 0 );

		if (! trans->uac[branch].reply ) {
			LOG(L_ERR, "ERROR: store_reply: can't alloc' clone memory\n");
			return 0;
		}

		return 1;
}



/* returns the number of authenticate replies (401 and 407) received so far
 *  (FAKED_REPLYes are excluded)
 *  It must be called with the REPLY_LOCK held */
inline static int auth_reply_count(struct cell *t, struct sip_msg* crt_reply)
{
	int count;
	int r;

	count=0;
	if (crt_reply && (crt_reply!=FAKED_REPLY) && 
			(crt_reply->REPLY_STATUS ==401 || crt_reply->REPLY_STATUS ==407))
		count=1;
	for (r=0; r<t->nr_of_outgoings; r++){
		if (t->uac[r].reply && (t->uac[r].reply!=FAKED_REPLY) &&
				(t->uac[r].last_received==401 || t->uac[r].last_received==407))
			count++;
	}
	return count;
}



/* must be called with the REPY_LOCK held */
inline static char* reply_aggregate_auth(int code, char* txt, str* new_tag, 
									struct cell* t, unsigned int* res_len, 
									struct bookmark* bm)
{
	int r;
	struct hdr_field* hdr;
	struct lump_rpl** first;
	struct lump_rpl** crt;
	struct lump_rpl* lst;
	struct lump_rpl*  lst_end;
	struct sip_msg* req;
	char* buf;
	
	first=0;
	lst_end=0;
	req=t->uas.request;
	
	for (r=0; r<t->nr_of_outgoings; r++){
		if (t->uac[r].reply && (t->uac[r].reply!=FAKED_REPLY) &&
			(t->uac[r].last_received==401 || t->uac[r].last_received==407)){
			for (hdr=t->uac[r].reply->headers; hdr; hdr=hdr->next){
				if (hdr->type==HDR_WWW_AUTHENTICATE_T ||
						hdr->type==HDR_PROXY_AUTHENTICATE_T){
					crt=add_lump_rpl2(req, hdr->name.s, hdr->len,
							LUMP_RPL_HDR|LUMP_RPL_NODUP|LUMP_RPL_NOFREE);
					if (crt==0){
						/* some kind of error, better stop */
						LOG(L_ERR, "ERROR: tm:reply_aggregate_auth:"
									" add_lump_rpl2 failed\n");
						goto skip;
					}
					lst_end=*crt;
					if (first==0) first=crt;
				}
			}
		}
	}
skip:
	buf=build_res_buf_from_sip_req(code, txt, new_tag, req, res_len, bm);
	/* clean the added lumps */
	if (first){
		lst=*first;
		*first=lst_end->next; /* "detach" the list of added rpl_lumps */
		lst_end->next=0; /* terminate lst */
		del_nonshm_lump_rpl(&lst);
		if (lst){
			LOG(L_CRIT, "BUG: tm: repply_aggregate_auth: rpl_lump list"
					    "contains shm alloc'ed lumps\n");
			abort();
		}
	}
	return buf;
}



/* this is the code which decides what and when shall be relayed
   upstream; note well -- it assumes it is entered locked with
   REPLY_LOCK and it returns unlocked!
   If do_put_on_wait==1 and this is the final reply, the transaction
   wait timer will be started (put_on_wait(t)).
*/
enum rps relay_reply( struct cell *t, struct sip_msg *p_msg, int branch,
	unsigned int msg_status, branch_bm_t *cancel_bitmap, int do_put_on_wait )
{
	int relay;
	int save_clone;
	char *buf;
	/* length of outbound reply */
	unsigned int res_len;
	int relayed_code;
	struct sip_msg *relayed_msg;
	struct sip_msg *reply_bak;
	struct bookmark bm;
	int totag_retr;
	enum rps reply_status;
	/* retransmission structure of outbound reply and request */
	struct retr_buf *uas_rb;
	str* to_tag;
#ifdef TMCB_ONSEND
	struct tmcb_params onsend_params;
#endif

	/* keep compiler warnings about use of uninit vars silent */
	res_len=0;
	buf=0;
	relayed_msg=0;
	relayed_code=0;
	totag_retr=0;


	/* remember, what was sent upstream to know whether we are
	 * forwarding a first final reply or not */

	/* *** store and relay message as needed *** */
	reply_status = t_should_relay_response(t, msg_status, branch,
		&save_clone, &relay, cancel_bitmap, p_msg );
	DBG("DEBUG: relay_reply: branch=%d, save=%d, relay=%d\n",
		branch, save_clone, relay );

	/* store the message if needed */
	if (save_clone) /* save for later use, typically branch picking */
	{
		if (!store_reply( t, branch, p_msg ))
			goto error01;
	}

	uas_rb = & t->uas.response;
	if (relay >= 0 ) {
		/* initialize sockets for outbound reply */
		uas_rb->activ_type=msg_status;
		/* only messages known to be relayed immediately will be
		 * be called on; we do not evoke this callback on messages
		 * stored in shmem -- they are fixed and one cannot change them
		 * anyway */
		if (unlikely(msg_status<300 && branch==relay
		&& has_tran_tmcbs(t,TMCB_RESPONSE_FWDED)) ) {
			run_trans_callbacks( TMCB_RESPONSE_FWDED, t, t->uas.request,
				p_msg, msg_status );
		}
		/* try building the outbound reply from either the current
		 * or a stored message */
		relayed_msg = branch==relay ? p_msg :  t->uac[relay].reply;
		if (relayed_msg==FAKED_REPLY) {
			relayed_code = branch==relay
				? msg_status : t->uac[relay].last_received;
			/* use to_tag from the original request, or if not present,
			 * generate a new one */
			if (relayed_code>=180 && t->uas.request->to
					&& (get_to(t->uas.request)->tag_value.s==0
			    		|| get_to(t->uas.request)->tag_value.len==0)) {
				calc_crc_suffix( t->uas.request, tm_tag_suffix );
				to_tag=&tm_tag;
			} else {
				to_tag=0;
			}
			if (cfg_get(tm, tm_cfg, tm_aggregate_auth) && 
						(relayed_code==401 || relayed_code==407) &&
						(auth_reply_count(t, p_msg)>1)){
				/* aggregate 401 & 407 www & proxy authenticate headers in
				 *  a "FAKE" reply*/
				
				/* temporarily "store" the current reply */
				reply_bak=t->uac[branch].reply;
				t->uac[branch].reply=p_msg;
				buf=reply_aggregate_auth(relayed_code, 
						error_text(relayed_code), to_tag, t, &res_len, &bm);
				/* revert the temporary "store" reply above */
				t->uac[branch].reply=reply_bak;
			}else{
				buf = build_res_buf_from_sip_req( relayed_code,
						error_text(relayed_code), to_tag,
						t->uas.request, &res_len, &bm );
			}
		} else {
			relayed_code=relayed_msg->REPLY_STATUS;
			if (relayed_code==503){
				/* replace a final 503 with a 500:
				 * generate a "FAKE" reply and a new to_tag (for easier
				 *  debugging)*/
				relayed_msg=FAKED_REPLY;
				if ((get_to(t->uas.request)->tag_value.s==0 ||
					 get_to(t->uas.request)->tag_value.len==0)) {
					calc_crc_suffix( t->uas.request, tm_tag_suffix );
					to_tag=&tm_tag;
				} else {
					to_tag=0;
				}
				/* don't relay a 503, replace it w/ 500 (rfc3261) */
				buf=build_res_buf_from_sip_req(500, error_text(relayed_code),
									to_tag, t->uas.request, &res_len, &bm);
				relayed_code=500;
			}else if (cfg_get(tm, tm_cfg, tm_aggregate_auth) && 
						(relayed_code==401 || relayed_code==407) &&
						(auth_reply_count(t, p_msg)>1)){
				/* aggregate 401 & 407 www & proxy authenticate headers in
				 *  a "FAKE" reply*/
				if ((get_to(t->uas.request)->tag_value.s==0 ||
					 get_to(t->uas.request)->tag_value.len==0)) {
					calc_crc_suffix( t->uas.request, tm_tag_suffix );
					to_tag=&tm_tag;
				} else {
					to_tag=0;
				}
				/* temporarily "store" the current reply */
				reply_bak=t->uac[branch].reply;
				t->uac[branch].reply=p_msg;
				buf=reply_aggregate_auth(relayed_code, 
						error_text(relayed_code), to_tag, t, &res_len, &bm);
				/* revert the temporary "store" reply above */
				t->uac[branch].reply=reply_bak;;
				relayed_msg=FAKED_REPLY; /* mark the relayed_msg as a "FAKE" */
			}else{
				buf = build_res_buf_from_sip_res( relayed_msg, &res_len );
				/* if we build a message from shmem, we need to remove
				   via delete lumps which are now stirred in the shmem-ed
				   structure
				*/
				if (branch!=relay) {
					free_via_clen_lump(&relayed_msg->add_rm);
				}
			}
		}
		update_reply_stats( relayed_code );
		if (!buf) {
			LOG(L_ERR, "ERROR: relay_reply: "
				"no mem for outbound reply buffer\n");
			goto error02;
		}

		/* attempt to copy the message to UAS's shmem:
		   - copy to-tag for ACK matching as well
		   -  allocate little a bit more for provisional as
		      larger messages are likely to follow and we will be
		      able to reuse the memory frag
		*/
		uas_rb->buffer = (char*)shm_resize( uas_rb->buffer, res_len +
			(msg_status<200 ?  REPLY_OVERBUFFER_LEN : 0));
		if (!uas_rb->buffer) {
			LOG(L_ERR, "ERROR: relay_reply: cannot alloc reply shmem\n");
			goto error03;
		}
		uas_rb->buffer_len = res_len;
		memcpy( uas_rb->buffer, buf, res_len );
		if (relayed_msg==FAKED_REPLY) { /* to-tags for local replies */
			update_local_tags(t, &bm, uas_rb->buffer, buf);
			t_stats_replied_locally();
		}
		
		/* update the status ... */
		t->uas.status = relayed_code;
		t->relayed_reply_branch = relay;

		if ( unlikely(is_invite(t) && relayed_msg!=FAKED_REPLY
		&& relayed_code>=200 && relayed_code < 300
		&& has_tran_tmcbs( t,
				TMCB_RESPONSE_OUT|TMCB_E2EACK_IN|TMCB_E2EACK_RETR_IN))) {
			totag_retr=update_totag_set(t, relayed_msg);
		}
	}; /* if relay ... */

	UNLOCK_REPLIES( t );

	/* send it now (from the private buffer) */
	if (relay >= 0) {
		/* Set retransmission timer before the reply is sent out to avoid
		* race conditions
		*
		* Call start_final_repl_retr/put_on_wait() only if we really send out
		* the reply. It can happen that the reply has been already sent from
		* failure_route or from a callback and the timer has been already
		* started. (Miklos)
		*/
		if (reply_status == RPS_COMPLETED) {
			start_final_repl_retr(t);
		}
		if (SEND_PR_BUFFER( uas_rb, buf, res_len )>=0){
			if (unlikely(!totag_retr && has_tran_tmcbs(t, TMCB_RESPONSE_OUT))){
				run_trans_callbacks( TMCB_RESPONSE_OUT, t, t->uas.request,
					relayed_msg, relayed_code);
			}
#ifdef TMCB_ONSEND
			if (unlikely(has_tran_tmcbs(t, TMCB_RESPONSE_SENT))){
				INIT_TMCB_ONSEND_PARAMS(onsend_params, t->uas.request,
									relayed_msg, uas_rb, &uas_rb->dst, buf,
									res_len,
									(relayed_msg==FAKED_REPLY)?TMCB_LOCAL_F:0,
									uas_rb->branch, relayed_code);
				run_onsend_callbacks2(TMCB_RESPONSE_SENT, t, &onsend_params);
			}
#endif
		}
		/* Call put_on_wait() only if we really send out
		* the reply. It can happen that the reply has been already sent from
		* failure_route  or from a callback and the timer has been already
		* started. (Miklos)
		*
		* put_on_wait() should always be called after we finished dealling 
		* with t, because otherwise the wait timer might fire before we
		*  finish with t, and by the time we want to use t it could
		*  be already deleted. This could happen only if this function is
		*  called from timer (fr_timer) (the timer doesn't refcnt) and the
		*  timer allows quick dels (timer_allow_del()). --andrei
		*/
		if (do_put_on_wait && (reply_status == RPS_COMPLETED)) {
			put_on_wait(t);
		}
		pkg_free( buf );
	}

	/* success */
	return reply_status;

error03:
	pkg_free( buf );
error02:
	if (save_clone) {
		if (t->uac[branch].reply!=FAKED_REPLY)
			sip_msg_free( t->uac[branch].reply );
		t->uac[branch].reply = NULL;
	}
error01:
	t_reply_unsafe( t, t->uas.request, 500, "Reply processing error" );
	*cancel_bitmap=0; /* t_reply_unsafe already canceled everything needed */
	UNLOCK_REPLIES(t);
	/* if (is_invite(t)) cancel_uacs( t, *cancel_bitmap, 0); 
	 *  -- not needed, t_reply_unsafe took care of this */

	/* a serious error occurred -- attempt to send an error reply;
	   it will take care of clean-ups  */

	/* failure */
	return RPS_ERROR;
}

/* this is the "UAC" above transaction layer; if a final reply
   is received, it triggers a callback; note well -- it assumes
   it is entered locked with REPLY_LOCK and it returns unlocked!
*/
enum rps local_reply( struct cell *t, struct sip_msg *p_msg, int branch,
	unsigned int msg_status, branch_bm_t *cancel_bitmap)
{
	/* how to deal with replies for local transaction */
	int local_store, local_winner;
	enum rps reply_status;
	struct sip_msg *winning_msg;
	int winning_code;
	int totag_retr;
	/* branch_bm_t cancel_bitmap; */

	/* keep warning 'var might be used un-inited' silent */
	winning_msg=0;
	winning_code=0;
	totag_retr=0;

	*cancel_bitmap=0;

	reply_status=t_should_relay_response( t, msg_status, branch,
		&local_store, &local_winner, cancel_bitmap, p_msg );
	DBG("DEBUG: local_reply: branch=%d, save=%d, winner=%d\n",
		branch, local_store, local_winner );
	if (local_store) {
		if (!store_reply(t, branch, p_msg))
			goto error;
	}
	if (local_winner>=0) {
		winning_msg= branch==local_winner
			? p_msg :  t->uac[local_winner].reply;
		if (winning_msg==FAKED_REPLY) {
			t_stats_replied_locally();
			winning_code = branch==local_winner
				? msg_status : t->uac[local_winner].last_received;
		} else {
			winning_code=winning_msg->REPLY_STATUS;
		}
		t->uas.status = winning_code;
		update_reply_stats( winning_code );
		if (unlikely(is_invite(t) && winning_msg!=FAKED_REPLY &&
					 winning_code>=200 && winning_code <300 &&
					 has_tran_tmcbs(t, TMCB_LOCAL_COMPLETED) ))  {
				totag_retr=update_totag_set(t, winning_msg);
		}
	}
	UNLOCK_REPLIES(t);
		
	if (local_winner >= 0
		&& cfg_get(tm, tm_cfg, pass_provisional_replies)
		&& winning_code < 200) {
			/* no retr. detection for provisional replies &
			 * TMCB_LOCAL_RESPONSE_OUT */
			if (unlikely(has_tran_tmcbs(t, TMCB_LOCAL_RESPONSE_OUT) )) {
				run_trans_callbacks( TMCB_LOCAL_RESPONSE_OUT, t, 0, 
										winning_msg, winning_code);
			}
	}
	
	if (local_winner>=0 && winning_code>=200 ) {
		DBG("DEBUG: local transaction completed\n");
		if (!totag_retr) {
			if (unlikely(has_tran_tmcbs(t,TMCB_LOCAL_COMPLETED) ))
				run_trans_callbacks( TMCB_LOCAL_COMPLETED, t, 0,
					winning_msg, winning_code );
		}
	}
	return reply_status;

error:
	which_cancel(t, cancel_bitmap);
	UNLOCK_REPLIES(t);
	cleanup_uac_timers(t);
	if ( get_cseq(p_msg)->method.len==INVITE_LEN
		&& memcmp( get_cseq(p_msg)->method.s, INVITE, INVITE_LEN)==0)
		cancel_uacs( t, *cancel_bitmap, F_CANCEL_B_KILL);
	*cancel_bitmap=0; /* we've already took care of everything */
	put_on_wait(t);
	return RPS_ERROR;
}





/*  This function is called whenever a reply for our module is received;
  * we need to register  this function on module initialization;
  *  Returns :   0 - core router stops
  *              1 - core router relay statelessly
  */
int reply_received( struct sip_msg  *p_msg )
{

	int msg_status;
	int last_uac_status;
	char *ack;
	unsigned int ack_len;
	int branch;
	/* has the transaction completed now and we need to clean-up? */
	int reply_status;
	branch_bm_t cancel_bitmap;
	struct ua_client *uac;
	struct cell *t;
	struct dest_info  lack_dst;
	avp_list_t* backup_user_from, *backup_user_to;
	avp_list_t* backup_domain_from, *backup_domain_to;
	avp_list_t* backup_uri_from, *backup_uri_to;
	struct run_act_ctx ra_ctx;
#ifdef USE_DNS_FAILOVER
	int branch_ret;
	int prev_branch;
#endif
#ifdef USE_DST_BLACKLIST
	int blst_503_timeout;
	struct dest_info src;
	struct hdr_field* hf;
#endif
#ifdef TMCB_ONSEND
	struct tmcb_params onsend_params;
#endif

	/* make sure we know the associated transaction ... */
	if (t_check( p_msg  , &branch )==-1)
		goto trans_not_found;
	/*... if there is none, tell the core router to fwd statelessly */
	t=get_t();
	if ( (t==0)||(t==T_UNDEFINED))
		goto trans_not_found;

	cancel_bitmap=0;
	msg_status=p_msg->REPLY_STATUS;

	uac=&t->uac[branch];
	DBG("DEBUG: reply_received: org. status uas=%d, "
		"uac[%d]=%d local=%d is_invite=%d)\n",
		t->uas.status, branch, uac->last_received,
		is_local(t), is_invite(t));
	last_uac_status=uac->last_received;

	/* it's a cancel ... ? */
	if (get_cseq(p_msg)->method.len==CANCEL_LEN
		&& memcmp( get_cseq(p_msg)->method.s, CANCEL, CANCEL_LEN)==0
		/* .. which is not e2e ? ... */
		&& is_invite(t) ) {
			/* ... then just stop timers */
			if ( msg_status >= 200 )
				stop_rb_timers(&uac->local_cancel); /* stop retr & fr */
			else
				stop_rb_retr(&uac->local_cancel);  /* stop only retr */
			DBG("DEBUG: reply to local CANCEL processed\n");
			goto done;
	}


	if ( msg_status >= 200 ){
		/* stop final response timer  & retr. only if I got a final response */
		stop_rb_timers(&uac->request); 
		/* acknowledge negative INVITE replies (do it before detailed
		 * on_reply processing, which may take very long, like if it
		 * is attempted to establish a TCP connection to a fail-over dst */
		if (is_invite(t)) {
			if (msg_status >= 300) {
				ack = build_ack(p_msg, t, branch, &ack_len);
				if (ack) {
#ifdef	TMCB_ONSEND
					if (SEND_PR_BUFFER(&uac->request, ack, ack_len)>=0)
						if (unlikely(has_tran_tmcbs(t, TMCB_REQUEST_SENT))){ 
							INIT_TMCB_ONSEND_PARAMS(onsend_params, 
									t->uas.request, p_msg, &uac->request,
									&uac->request.dst, ack, ack_len,
									TMCB_LOCAL_F, branch, TYPE_LOCAL_ACK);
							run_onsend_callbacks2(TMCB_REQUEST_SENT, t,
													&onsend_params);
						}
#else
					SEND_PR_BUFFER(&uac->request, ack, ack_len);
#endif
					shm_free(ack);
				}
			} else if (is_local(t) /*&& 200 <= msg_status < 300*/) {
				ack = build_local_ack(p_msg, t, branch, &ack_len, &lack_dst);
				if (ack) {
					if (msg_send(&lack_dst, ack, ack_len)<0)
						LOG(L_ERR, "Error while sending local ACK\n");
#ifdef	TMCB_ONSEND
					else if (unlikely(has_tran_tmcbs(t, TMCB_REQUEST_SENT))){
							INIT_TMCB_ONSEND_PARAMS(onsend_params, 
									t->uas.request, p_msg, &uac->request,
									&lack_dst, ack, ack_len, TMCB_LOCAL_F,
									branch, TYPE_LOCAL_ACK);
							run_onsend_callbacks2(TMCB_REQUEST_SENT, t,
													&onsend_params);
					}
#endif
					shm_free(ack);
				}
			}
		}
	}else{
		/* if branch already canceled re-transmit or generate cancel
		 * TODO: check if it really makes sense to do it for non-invites too */
		if (uac->request.flags & F_RB_CANCELED){
			if (uac->local_cancel.buffer_len){
				membar_read(); /* make sure we get the current value of
								  local_cancel */
				/* re-transmit if cancel already built */
				DBG("tm: reply_received: branch CANCEL retransmit\n");
#ifdef TMCB_ONSEND
				if (SEND_BUFFER( &uac->local_cancel)>=0){
					if (unlikely (has_tran_tmcbs(t, TMCB_REQUEST_SENT)))
						run_onsend_callbacks(TMCB_REQUEST_SENT,
											&uac->local_cancel,
											0, 0, TMCB_LOCAL_F);
				}
#else
				SEND_BUFFER( &uac->local_cancel );
#endif
				/* retrs. should be already started so do nothing */
			}else if (atomic_cmpxchg_long((void*)&uac->local_cancel.buffer, 0, 
										(long)BUSY_BUFFER)==0){
				/* try to rebuild it if empty (not set or marked as BUSY).
				 * if BUSY or set just exit, a cancel will be (or was) sent 
				 * shortly on this branch */
				DBG("tm: reply_received: branch CANCEL created\n");
				cancel_branch(t, branch, F_CANCEL_B_FORCE_C);
			}
			goto done; /* nothing to do */
		}
		if (is_invite(t)){
			/* stop only retr. (and not fr) */
			stop_rb_retr(&uac->request);
		}else{
			/* non-invite: increase retransmissions interval (slow now) */
			switch_rb_retr_to_t2(&uac->request);
		}
	}
	/* processing of on_reply block */
	if (t->on_reply) {
		rmode=MODE_ONREPLY;
		/* transfer transaction flag to message context */
		if (t->uas.request) p_msg->flags=t->uas.request->flags;
		/* set the as avp_list the one from transaction */

		backup_uri_from = set_avp_list(AVP_TRACK_FROM | AVP_CLASS_URI, &t->uri_avps_from );
		backup_uri_to = set_avp_list(AVP_TRACK_TO | AVP_CLASS_URI, &t->uri_avps_to );
		backup_user_from = set_avp_list(AVP_TRACK_FROM | AVP_CLASS_USER, &t->user_avps_from );
		backup_user_to = set_avp_list(AVP_TRACK_TO | AVP_CLASS_USER, &t->user_avps_to );
		backup_domain_from = set_avp_list(AVP_TRACK_FROM | AVP_CLASS_DOMAIN, &t->domain_avps_from );
		backup_domain_to = set_avp_list(AVP_TRACK_TO | AVP_CLASS_DOMAIN, &t->domain_avps_to );
		reset_static_buffer();
		init_run_actions_ctx(&ra_ctx);
		if (run_actions(&ra_ctx, onreply_rt.rlist[t->on_reply], p_msg)<0)
			LOG(L_ERR, "ERROR: on_reply processing failed\n");
		/* transfer current message context back to t */
		if (t->uas.request) t->uas.request->flags=p_msg->flags;
		/* restore original avp list */
		set_avp_list( AVP_TRACK_FROM | AVP_CLASS_URI, backup_uri_from );
		set_avp_list( AVP_TRACK_TO | AVP_CLASS_URI, backup_uri_to );
		set_avp_list( AVP_TRACK_FROM | AVP_CLASS_USER, backup_user_from );
		set_avp_list( AVP_TRACK_TO | AVP_CLASS_USER, backup_user_to );
		set_avp_list( AVP_TRACK_FROM | AVP_CLASS_DOMAIN, backup_domain_from );
		set_avp_list( AVP_TRACK_TO | AVP_CLASS_DOMAIN, backup_domain_to );
	}
#ifdef USE_DST_BLACKLIST
		/* add temporary to the blacklist the source of a 503 reply */
		if (cfg_get(tm, tm_cfg, tm_blst_503)
			&& cfg_get(core, core_cfg, use_dst_blacklist)
			&& (msg_status==503)
		){
			blst_503_timeout=cfg_get(tm, tm_cfg, tm_blst_503_default);
			if ((parse_headers(p_msg, HDR_RETRY_AFTER_F, 0)==0) && 
				(p_msg->parsed_flag & HDR_RETRY_AFTER_F)){
				for (hf=p_msg->headers; hf; hf=hf->next)
					if (hf->type==HDR_RETRY_AFTER_T){
						/* found */
						blst_503_timeout=(unsigned)(unsigned long)hf->parsed;
						blst_503_timeout=MAX_unsigned(blst_503_timeout, 
									cfg_get(tm, tm_cfg, tm_blst_503_min));
						blst_503_timeout=MIN_unsigned(blst_503_timeout,
									cfg_get(tm, tm_cfg, tm_blst_503_max));
						break;
					}
			}
			if (blst_503_timeout){
				src.send_sock=0;
				src.to=p_msg->rcv.src_su;
				src.id=p_msg->rcv.proto_reserved1;
				src.proto=p_msg->rcv.proto;
				dst_blacklist_add_to(BLST_503, &src,  p_msg, 
									S_TO_TICKS(blst_503_timeout));
			}
		}
#endif /* USE_DST_BLACKLIST */
#ifdef USE_DNS_FAILOVER
		/* if this is a 503 reply, and the destination resolves to more ips,
		 *  add another branch/uac.
		 *  This code is out of LOCK_REPLIES() to minimize the time the
		 *  reply lock is held (the lock won't be held while sending the
		 *   message)*/
		if (cfg_get(core, core_cfg, use_dns_failover) && (msg_status==503)) {
			branch_ret=add_uac_dns_fallback(t, t->uas.request, uac, 1);
			prev_branch=-1;
			while((branch_ret>=0) &&(branch_ret!=prev_branch)){
				prev_branch=branch_ret;
				branch_ret=t_send_branch(t, branch_ret, t->uas.request , 0, 1);
			}
		}
#endif
	LOCK_REPLIES( t );
	if ( is_local(t) ) {
		reply_status=local_reply( t, p_msg, branch, msg_status, &cancel_bitmap );
		if (reply_status == RPS_COMPLETED) {
			     /* no more UAC FR/RETR (if I received a 2xx, there may
			      * be still pending branches ...
			      */
			cleanup_uac_timers( t );
			if (is_invite(t)) cancel_uacs(t, cancel_bitmap, F_CANCEL_B_KILL);
			/* There is no need to call set_final_timer because we know
			 * that the transaction is local */
			put_on_wait(t);
		}else if (cancel_bitmap){
			/* cancel everything, even non-INVITEs (e.g in case of 6xx), use
			 * cancel_b_method for canceling unreplied branches */
			cancel_uacs(t, cancel_bitmap, cfg_get(tm,tm_cfg, cancel_b_flags));
		}
	} else {
		reply_status=relay_reply( t, p_msg, branch, msg_status,
									&cancel_bitmap, 1 );
		if (reply_status == RPS_COMPLETED) {
			     /* no more UAC FR/RETR (if I received a 2xx, there may
				be still pending branches ...
			     */
			cleanup_uac_timers( t );
			/* 2xx is a special case: we can have a COMPLETED request
			 * with branches still open => we have to cancel them */
			if (is_invite(t) && cancel_bitmap) 
				cancel_uacs( t, cancel_bitmap,  F_CANCEL_B_KILL);
			/* FR for negative INVITES, WAIT anything else */
			/* Call to set_final_timer is embedded in relay_reply to avoid
			 * race conditions when reply is sent out and an ACK to stop
			 * retransmissions comes before retransmission timer is set.*/
		}else if (cancel_bitmap){
			/* cancel everything, even non-INVITEs (e.g in case of 6xx), use
			 * cancel_b_method for canceling unreplied branches */
			cancel_uacs(t, cancel_bitmap, cfg_get(tm,tm_cfg, cancel_b_flags));
		}
	}
	uac->request.flags|=F_RB_REPLIED;

	if (reply_status==RPS_ERROR)
		goto done;

	/* update FR/RETR timers on provisional replies */
	if (is_invite(t) && msg_status<200 &&
		( cfg_get(tm, tm_cfg, restart_fr_on_each_reply) ||
				( (last_uac_status<msg_status) &&
					((msg_status>=180) || (last_uac_status==0)) )
			) ) { /* provisional now */
		restart_rb_fr(& uac->request, t->fr_inv_timeout);
		uac->request.flags|=F_RB_FR_INV; /* mark fr_inv */
	} /* provisional replies */

done:
	/* we are done with the transaction, so unref it - the reference
	 * was incremented by t_check() function -bogdan*/
	t_unref(p_msg);
	/* don't try to relay statelessly neither on success
       (we forwarded statefully) nor on error; on troubles,
	   simply do nothing; that will make the other party to
	   retransmit; hopefuly, we'll then be better off */
	return 0;

trans_not_found:
	/* transaction context was not found */
	if (goto_on_sl_reply) {
		/* the script writer has a chance to decide whether to
		forward the reply or not */
		init_run_actions_ctx(&ra_ctx);
		return run_actions(&ra_ctx, onreply_rt.rlist[goto_on_sl_reply], p_msg);
	} else {
		/* let the core forward the reply */
		return 1;
	}
}



int t_reply_with_body( struct cell *trans, unsigned int code,
		char * text, char * body, char * new_header, char * to_tag )
{
	struct lump_rpl *hdr_lump;
	struct lump_rpl *body_lump;
	str  s_to_tag;
	str  rpl;
	int  ret;
	struct bookmark bm;

	s_to_tag.s = to_tag;
	if(to_tag)
		s_to_tag.len = strlen(to_tag);
	else
		s_to_tag.len = 0;

	/* mark the transaction as replied */
	if (code>=200) set_kr(REQ_RPLD);

	/* add the lumps for new_header and for body (by bogdan) */
	if (new_header && strlen(new_header)) {
		hdr_lump = add_lump_rpl( trans->uas.request, new_header,
					 strlen(new_header), LUMP_RPL_HDR );
		if ( !hdr_lump ) {
			LOG(L_ERR,"ERROR:tm:t_reply_with_body: cannot add hdr lump\n");
			goto error;
		}
	} else {
		hdr_lump = 0;
	}

	/* body lump */
	if(body && strlen(body)) {
		body_lump = add_lump_rpl( trans->uas.request, body, strlen(body),
			LUMP_RPL_BODY );
		if (body_lump==0) {
			LOG(L_ERR,"ERROR:tm:t_reply_with_body: cannot add body lump\n");
			goto error_1;
		}
	} else {
		body_lump = 0;
	}

	rpl.s = build_res_buf_from_sip_req(
			code, text, &s_to_tag,
			trans->uas.request, (unsigned int*)&rpl.len, &bm);

	/* since the msg (trans->uas.request) is a clone into shm memory, to avoid
	 * memory leak or crashing (lumps are create in private memory) I will
	 * remove the lumps by myself here (bogdan) */
	if ( hdr_lump ) {
		unlink_lump_rpl( trans->uas.request, hdr_lump);
		free_lump_rpl( hdr_lump );
	}
	if( body_lump ) {
		unlink_lump_rpl( trans->uas.request, body_lump);
		free_lump_rpl( body_lump );
	}

	if (rpl.s==0) {
		LOG(L_ERR,"ERROR:tm:t_reply_with_body: failed in doing "
			"build_res_buf_from_sip_req()\n");
		goto error;
	}

	DBG("t_reply_with_body: buffer computed\n");
	// frees 'res.s' ... no panic !
	ret=_reply_light( trans, rpl.s, rpl.len, code, text,
		s_to_tag.s, s_to_tag.len, 1 /* lock replies */, &bm );
	/* this is ugly hack -- the function caller may wish to continue with
	 * transaction and I unref; however, there is now only one use from
	 * vm/fifo_vm_reply and I'm currently to lazy to export UNREF; -jiri
	 */
	UNREF(trans);

	return ret;
error_1:
	if ( hdr_lump ) {
		unlink_lump_rpl( trans->uas.request, hdr_lump);
		free_lump_rpl( hdr_lump );
	}
error:
	return -1;
}

/* drops all the replies to make sure
 * that none of them is picked up again
 */
void t_drop_replies(void)
{
	/* It is too risky to free the replies that are in shm mem
	at the middle of failure_route block, because other functions might
	need them as well. And it can also happen that the current reply is not yet
	in shm mem, we are just going to clone it. So better to set a flag
	and check it after failure_route has ended. (Miklos) */
	drop_replies = 1;
}

#if 0
static int send_reply(struct cell *trans, unsigned int code, str* text, str* body, str* headers, str* to_tag)
{
	struct lump_rpl *hdr_lump, *body_lump;
	str rpl;
	int ret;
	struct bookmark bm;

	     /* mark the transaction as replied */
	if (code >= 200) set_kr(REQ_RPLD);

	     /* add the lumps for new_header and for body (by bogdan) */
	if (headers && headers->len) {
		hdr_lump = add_lump_rpl(trans->uas.request, headers->s, headers->len, LUMP_RPL_HDR);
		if (!hdr_lump) {
			LOG(L_ERR, "send_reply: cannot add hdr lump\n");
			goto sr_error;
		}
	} else {
		hdr_lump = 0;
	}

	     /* body lump */
	if (body && body->len) {
		body_lump = add_lump_rpl(trans->uas.request, body->s, body->len, LUMP_RPL_BODY);
		if (body_lump == 0) {
			LOG(L_ERR,"send_reply: cannot add body lump\n");
			goto sr_error_1;
		}
	} else {
		body_lump = 0;
	}

	     /* We can safely zero-terminate the text here, because it is followed
	      * by next line in the received message
	      */
	text->s[text->len] = '\0';
	rpl.s = build_res_buf_from_sip_req(code, text->s, to_tag, trans->uas.request, (unsigned int*)&rpl.len, &bm);

	     /* since the msg (trans->uas.request) is a clone into shm memory, to avoid
	      * memory leak or crashing (lumps are create in private memory) I will
	      * remove the lumps by myself here (bogdan) */
	if (hdr_lump) {
		unlink_lump_rpl(trans->uas.request, hdr_lump);
		free_lump_rpl(hdr_lump);
	}
	if (body_lump) {
		unlink_lump_rpl(trans->uas.request, body_lump);
		free_lump_rpl(body_lump);
	}

	if (rpl.s == 0) {
		LOG(L_ERR,"send_reply: failed in build_res_buf_from_sip_req\n");
		goto sr_error;
	}

	ret = _reply_light(trans, rpl.s, rpl.len, code, text->s,  to_tag->s, to_tag->len, 1 /* lock replies */, &bm);
	     /* this is ugly hack -- the function caller may wish to continue with
	      * transaction and I unref; however, there is now only one use from
	      * vm/fifo_vm_reply and I'm currently to lazy to export UNREF; -jiri
	      */
	UNREF(trans);
	return ret;
 sr_error_1:
	if (hdr_lump) {
		unlink_lump_rpl(trans->uas.request, hdr_lump);
		free_lump_rpl(hdr_lump);
	}
 sr_error:
	return -1;
}
#endif


const char* rpc_reply_doc[2] = {
	"Reply transaction",
	0
};

/*
  Syntax:

  ":tm.reply:[response file]\n
  code\n
  reason\n
  trans_id\n
  to_tag\n
  [new headers]\n
  \n
  [Body]\n
  .\n
  \n"
 */
void rpc_reply(rpc_t* rpc, void* c)
{
	int ret;
	struct cell *trans;
	unsigned int hash_index, label, code;
	str ti;
	char* reason, *body, *headers, *tag;

	if (rpc->scan(c, "d", &code) < 1) {
		rpc->fault(c, 400, "Reply code expected");
		return;
	}

	if (rpc->scan(c, "s", &reason) < 1) {
		rpc->fault(c, 400, "Reason phrase expected");
		return;
	}

	if (rpc->scan(c, "s", &ti.s) < 1) {
		rpc->fault(c, 400, "Transaction ID expected");
		return;
	}
	ti.len = strlen(ti.s);

	if (rpc->scan(c, "s", &tag) < 1) {
		rpc->fault(c, 400, "To tag expected");
		return;
	}

	if (rpc->scan(c, "s", &headers) < 0) return;
	if (rpc->scan(c, "s", &body) < 0) return;

	if(sscanf(ti.s,"%u:%u", &hash_index, &label) != 2) {
		ERR("Invalid trans_id (%s)\n", ti.s);
		rpc->fault(c, 400, "Invalid transaction ID");
		return;
	}
	DBG("hash_index=%u label=%u\n", hash_index, label);

	if( t_lookup_ident(&trans, hash_index, label) < 0 ) {
		ERR("Lookup failed\n");
		rpc->fault(c, 481, "No such transaction");
		return;
	}

	/* it's refcounted now, t_reply_with body unrefs for me -- I can
	 * continue but may not use T anymore  */
	ret = t_reply_with_body(trans, code, reason, body, headers, tag);

	if (ret < 0) {
		ERR("Reply failed\n");
		rpc->fault(c, 500, "Reply failed");
		return;
	}
}
