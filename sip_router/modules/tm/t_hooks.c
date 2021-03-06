/*
 * $Id: t_hooks.c,v 1.31 2007/05/29 13:44:19 andrei Exp $
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
 * --------
 *  2003-03-19  replaced all the mallocs/frees w/ pkg_malloc/pkg_free (andrei)
 *  2003-12-04  global callbacks moved into transaction callbacks;
 *              multiple events per callback added; single list per
 *              transaction for all its callbacks (bogdan)
 *  2004-08-23  user avp(attribute value pair) added -> making avp list
 *              available in callbacks (bogdan)
 * 2007-03-08  membar_write() used in insert_tmcb(...) (andrei)
 * 2007-03-14  added *_SENT callbacks (andrei)
 * 2007-03-23  added local_req_in callbacks support (andrei)
 * 2007-05-17  insert_tmcb is now safe: it loops arround a mb_atomic_cmpxchg
 *              for a safe lockless list insert (andrei)
 */

#include "defs.h"


#include "stdlib.h"
#include "../../dprint.h"
#include "../../error.h"
#include "../../mem/mem.h"
#include "../../usr_avp.h"
#include "../../atomic_ops.h" /* membar_write() */
#include "t_hooks.h"
#include "t_lookup.h"
#include "t_funcs.h"


struct tmcb_head_list* req_in_tmcb_hl = 0;
struct tmcb_head_list* local_req_in_tmcb_hl = 0;



int init_tmcb_lists()
{
	req_in_tmcb_hl = (struct tmcb_head_list*)shm_malloc
		( sizeof(struct tmcb_head_list) );
	local_req_in_tmcb_hl = (struct tmcb_head_list*)shm_malloc
		( sizeof(struct tmcb_head_list) );
	if ((req_in_tmcb_hl==0) || (local_req_in_tmcb_hl==0)) {
		LOG(L_CRIT,"ERROR:tm:init_tmcb_lists: no more shared mem\n");
		goto error;
	}
	req_in_tmcb_hl->first = 0;
	req_in_tmcb_hl->reg_types = 0;
	local_req_in_tmcb_hl->first = 0;
	local_req_in_tmcb_hl->reg_types = 0;
	return 1;
error:
	if (req_in_tmcb_hl){
		shm_free(req_in_tmcb_hl);
		req_in_tmcb_hl=0;
	}
	if(local_req_in_tmcb_hl){
		shm_free(local_req_in_tmcb_hl);
		local_req_in_tmcb_hl=0;
	}
	return -1;
}


void destroy_tmcb_lists()
{
	struct tm_callback *cbp, *cbp_tmp;

	if (req_in_tmcb_hl){
		for( cbp=(struct tm_callback*)req_in_tmcb_hl->first; cbp ; ) {
			cbp_tmp = cbp;
			cbp = cbp->next;
			if (cbp_tmp->param) shm_free( cbp_tmp->param );
			shm_free( cbp_tmp );
		}
		shm_free(req_in_tmcb_hl);
		req_in_tmcb_hl=0;
	}
	if(local_req_in_tmcb_hl){
		for( cbp=(struct tm_callback*)local_req_in_tmcb_hl->first; cbp ; ) {
			cbp_tmp = cbp;
			cbp = cbp->next;
			if (cbp_tmp->param) shm_free( cbp_tmp->param );
			shm_free( cbp_tmp );
		}
		shm_free(local_req_in_tmcb_hl);
		local_req_in_tmcb_hl=0;
	}
}



/* lockless insert: should be always safe */
int insert_tmcb(struct tmcb_head_list *cb_list, int types,
									transaction_cb f, void *param )
{
	struct tm_callback *cbp;
	struct tm_callback *old;


	/* build a new callback structure */
	if (!(cbp=shm_malloc( sizeof( struct tm_callback)))) {
		LOG(L_ERR, "ERROR:tm:insert_tmcb: out of shm. mem\n");
		return E_OUT_OF_MEM;
	}

	atomic_or_int(&cb_list->reg_types, types);
	/* ... and fill it up */
	cbp->callback = f;
	cbp->param = param;
	cbp->types = types;
	cbp->id=0;
	old=(struct tm_callback*)cb_list->first;
	/* link it into the proper place... */
	do{
		cbp->next = old;
		/*
		if (cbp->next)
			cbp->id = cbp->next->id+1;
		else
			cbp->id = 0;
		 -- callback ids are useless -- andrei */
		membar_write_atomic_op();
		old=(void*)atomic_cmpxchg_long((void*)&cb_list->first,
										(long)old, (long)cbp);
	}while(old!=cbp->next);

	return 1;
}



/* register a callback function 'f' for 'types' mask of events;
 * will be called back whenever one of the events occurs in transaction module
 * (global or per transaction, depending of event type)
 * It _must_ be always called either with the REPLY_LOCK held, or before the
 *  branches are created.
 *  Special cases: TMCB_REQUEST_IN & TMCB_LOCAL_REQUEST_IN - must be called 
 *                 from mod_init (before forking!).
*/
int register_tmcb( struct sip_msg* p_msg, struct cell *t, int types,
											transaction_cb f, void *param )
{
	//struct cell* t;
	struct tmcb_head_list *cb_list;

	/* are the callback types valid?... */
	if ( types<0 || types>TMCB_MAX ) {
		LOG(L_CRIT, "BUG:tm:register_tmcb: invalid callback types: mask=%d\n",
			types);
		return E_BUG;
	}
	/* we don't register null functions */
	if (f==0) {
		LOG(L_CRIT, "BUG:tm:register_tmcb: null callback function\n");
		return E_BUG;
	}

	if (types&TMCB_REQUEST_IN) {
		if (types!=TMCB_REQUEST_IN) {
			LOG(L_CRIT, "BUG:tm:register_tmcb: callback type TMCB_REQUEST_IN "
				"can't be register along with types\n");
			return E_BUG;
		}
		cb_list = req_in_tmcb_hl;
	}else if (types & TMCB_LOCAL_REQUEST_IN) {
		if (types!=TMCB_LOCAL_REQUEST_IN) {
			LOG(L_CRIT, "BUG:tm:register_tmcb: callback type"
					" TMCB_LOCAL_REQUEST_IN can't be register along with"
					" other types\n");
			return E_BUG;
		}
		cb_list = local_req_in_tmcb_hl;
	} else {
		if (!t) {
			if (!p_msg) {
				LOG(L_CRIT,"BUG:tm:register_tmcb: no sip_msg, nor transaction"
					" given\n");
				return E_BUG;
			}
			/* look for the transaction */
			if ( t_check(p_msg,0)!=1 ){
				LOG(L_CRIT,"BUG:tm:register_tmcb: no transaction found\n");
				return E_BUG;
			}
			if ( (t=get_t())==0 ) {
				LOG(L_CRIT,"BUG:tm:register_tmcb: transaction found "
					"is NULL\n");
				return E_BUG;
			}
		}
		cb_list = &(t->tmcb_hl);
	}

	return insert_tmcb( cb_list, types, f, param );
}


void run_trans_callbacks_internal(struct tmcb_head_list* cb_lst, int type,
									struct cell *trans, 
									struct tmcb_params *params)
{
	struct tm_callback    *cbp;
	avp_list_t* backup_from, *backup_to, *backup_dom_from, *backup_dom_to, *backup_uri_from, *backup_uri_to;

	backup_uri_from = set_avp_list(AVP_CLASS_URI | AVP_TRACK_FROM,
			&trans->uri_avps_from );
	backup_uri_to = set_avp_list(AVP_CLASS_URI | AVP_TRACK_TO, 
			&trans->uri_avps_to );
	backup_from = set_avp_list(AVP_CLASS_USER | AVP_TRACK_FROM, 
			&trans->user_avps_from );
	backup_to = set_avp_list(AVP_CLASS_USER | AVP_TRACK_TO, 
			&trans->user_avps_to );
	backup_dom_from = set_avp_list(AVP_CLASS_DOMAIN | AVP_TRACK_FROM, 
			&trans->domain_avps_from);
	backup_dom_to = set_avp_list(AVP_CLASS_DOMAIN | AVP_TRACK_TO, 
			&trans->domain_avps_to);
	cbp=(struct tm_callback*)cb_lst->first;
	while(cbp){
		membar_depends(); /* make sure the cache has the correct cbp 
							 contents */
		if ( (cbp->types)&type ) {
			DBG("DBG: trans=%p, callback type %d, id %d entered\n",
				trans, type, cbp->id );
			params->param = &(cbp->param);
			cbp->callback( trans, type, params );
		}
		cbp=cbp->next;
	}
	set_avp_list(AVP_CLASS_DOMAIN | AVP_TRACK_TO, backup_dom_to );
	set_avp_list(AVP_CLASS_DOMAIN | AVP_TRACK_FROM, backup_dom_from );
	set_avp_list(AVP_CLASS_USER | AVP_TRACK_TO, backup_to );
	set_avp_list(AVP_CLASS_USER | AVP_TRACK_FROM, backup_from );
	set_avp_list(AVP_CLASS_URI | AVP_TRACK_TO, backup_uri_to );
	set_avp_list(AVP_CLASS_URI | AVP_TRACK_FROM, backup_uri_from );
}



void run_trans_callbacks( int type , struct cell *trans,
						struct sip_msg *req, struct sip_msg *rpl, int code )
{
	struct tmcb_params params;
	if (trans->tmcb_hl.first==0 || ((trans->tmcb_hl.reg_types)&type)==0 )
		return;
	memset (&params, 0, sizeof(params));
	params.req = req;
	params.rpl = rpl;
	params.code = code;
	run_trans_callbacks_internal(&trans->tmcb_hl, type, trans, &params);
}



#ifdef TMCB_ONSEND
void run_onsend_callbacks(int type, struct retr_buf* rbuf,
					struct sip_msg* req, struct sip_msg* repl, short flags)
{
	struct tmcb_params params;
	struct cell * trans;

	trans=rbuf->my_T;
	if ( trans==0 || trans->tmcb_hl.first==0 || 
			((trans->tmcb_hl.reg_types)&type)==0 )
		return;
	INIT_TMCB_ONSEND_PARAMS(params, req, repl, rbuf, &rbuf->dst, rbuf->buffer,
					rbuf->buffer_len, flags, rbuf->branch, rbuf->activ_type);
	/* req, rpl */
	run_trans_callbacks_internal(&trans->tmcb_hl, type, trans, &params);
}


void run_onsend_callbacks2(int type, struct cell* trans, struct tmcb_params* p)
{

	if (p->t_rbuf==0) return;
	if ( trans==0 || trans->tmcb_hl.first==0 || 
			((trans->tmcb_hl.reg_types)&type)==0 )
		return;
	run_trans_callbacks_internal(&trans->tmcb_hl, type, p->t_rbuf->my_T, p);
}

#endif

static void run_reqin_callbacks_internal(struct tmcb_head_list* hl,
							struct cell *trans, struct tmcb_params* params)
{
	struct tm_callback    *cbp;
	avp_list_t* backup_from, *backup_to, *backup_dom_from, *backup_dom_to,
				*backup_uri_from, *backup_uri_to;

	if (hl==0 || hl->first==0) return;
	backup_uri_from = set_avp_list(AVP_CLASS_URI | AVP_TRACK_FROM,
			&trans->uri_avps_from );
	backup_uri_to = set_avp_list(AVP_CLASS_URI | AVP_TRACK_TO, 
			&trans->uri_avps_to );
	backup_from = set_avp_list(AVP_CLASS_USER | AVP_TRACK_FROM, 
			&trans->user_avps_from );
	backup_to = set_avp_list(AVP_CLASS_USER | AVP_TRACK_TO, 
			&trans->user_avps_to );
	backup_dom_from = set_avp_list(AVP_CLASS_DOMAIN | AVP_TRACK_FROM, 
			&trans->domain_avps_from);
	backup_dom_to = set_avp_list(AVP_CLASS_DOMAIN | AVP_TRACK_TO, 
			&trans->domain_avps_to);
	for (cbp=(struct tm_callback*)hl->first; cbp; cbp=cbp->next)  {
		DBG("DBG: trans=%p, callback type %d, id %d entered\n",
			trans, cbp->types, cbp->id );
		params->param = &(cbp->param);
		cbp->callback( trans, cbp->types, params );
	}
	set_avp_list(AVP_CLASS_URI | AVP_TRACK_TO, backup_uri_to );
	set_avp_list(AVP_CLASS_URI | AVP_TRACK_FROM, backup_uri_from );
	set_avp_list(AVP_CLASS_DOMAIN | AVP_TRACK_TO, backup_dom_to );
	set_avp_list(AVP_CLASS_DOMAIN | AVP_TRACK_FROM, backup_dom_from );
	set_avp_list(AVP_CLASS_USER | AVP_TRACK_TO, backup_to );
	set_avp_list(AVP_CLASS_USER | AVP_TRACK_FROM, backup_from );
}



void run_reqin_callbacks( struct cell *trans, struct sip_msg *req, int code )
{
	static struct tmcb_params params;

	if (req_in_tmcb_hl->first==0)
		return;
	memset (&params, 0, sizeof(params));
	params.req = req;
	params.code = code;
	
	run_reqin_callbacks_internal(req_in_tmcb_hl, trans, &params);
}


void run_local_reqin_callbacks( struct cell *trans, struct sip_msg *req,
								int code )
{
	static struct tmcb_params params;

	if (local_req_in_tmcb_hl->first==0)
		return;
	memset (&params, 0, sizeof(params));
	params.req = req;
	params.code = code;
	
	run_reqin_callbacks_internal(local_req_in_tmcb_hl, trans, &params);
}
