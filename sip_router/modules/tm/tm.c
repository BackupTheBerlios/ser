/*
 * $Id: tm.c,v 1.132 2006/01/10 14:59:44 janakj Exp $
 *
 * TM module
 *
 *
 * ***************************************************
 * * Jiri's Source Memorial                          *
 * *                                                 *
 * * Welcome, pilgrim ! This is the greatest place   *
 * * where dramatic changes happend. There are not   *
 * * many places with a history like this, as there  *
 * * are not so many people like Jiri, one of the    *
 * * ser's fathers, who brought everywhere the wind  *
 * * of change, the flood of clean-up. We all felt   *
 * * his fatherly eye watching over us day and night.*
 * *                                                 *
 * * Please, preserve this codework heritage, as     *
 * * it's unlikely for fresh, juicy pieces of code to  *
 * * arise to give him the again the chance to       *
 * * demonstrate his clean-up and improvement skills.*
 * *                                                 *
 * * Hereby, we solicit you to adopt this historical *
 * * piece of code. For $100, your name will be      *
 * * be printed in this banner and we will use       *
 * * collected funds to create and display an ASCII  *
 * * statue of Jiri  .                               *
 * ***************************************************
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
 * --------
 *  2003-02-18  added t_forward_nonack_{udp, tcp}, t_relay_to_{udp,tcp},
 *               t_replicate_{udp, tcp} (andrei)
 *  2003-02-19  added t_rely_{udp, tcp} (andrei)
 *  2003-03-06  voicemail changes accepted (jiri)
 *  2003-03-10  module export interface updated to the new format (andrei)
 *  2003-03-16  flags export parameter added (janakj)
 *  2003-03-19  replaced all mallocs/frees w/ pkg_malloc/pkg_free (andrei)
 *  2003-03-30  set_kr for requests only (jiri)
 *  2003-04-05  s/reply_route/failure_route, onreply_route introduced (jiri)
 *  2003-04-14  use protocol from uri (jiri)
 *  2003-07-07  added t_relay_to_tls, t_replicate_tls, t_forward_nonack_tls
 *              added #ifdef USE_TCP, USE_TLS
 *              removed t_relay_{udp,tcp,tls} (andrei)
 *  2003-09-26  added t_forward_nonack_uri() - same as t_forward_nonack() but
 *              takes no parameters -> forwards to uri (bogdan)
 *  2004-02-11  FIFO/CANCEL + alignments (hash=f(callid,cseq)) (uli+jiri)
 *  2004-02-18  t_reply exported via FIFO - imported from VM (bogdan)
 *  2004-10-01  added a new param.: restart_fr_on_each_reply (andrei)
 *  2005-11-14  new timer support, changed timer related module params (andrei)
 *  2005-12-09  fixup_hostport2proxy uses route_struct to access param #1
 *              when fixing param #2
 *  2005-12-09  added t_set_fr() (andrei)
 */


#include "defs.h"


#include <stdio.h>
#include <string.h>
#include <netdb.h>

#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../error.h"
#include "../../ut.h"
#include "../../script_cb.h"
#include "../../usr_avp.h"
#include "../../mem/mem.h"
#include "../../route_struct.h"

#include "sip_msg.h"
#include "h_table.h"
#include "t_hooks.h"
#include "tm_load.h"
#include "ut.h"
#include "t_reply.h"
#include "uac.h"
#include "t_fwd.h"
#include "t_lookup.h"
#include "t_stats.h"
#include "callid.h"
#include "t_cancel.h"
#include "t_fifo.h"
#include "timer.h"

MODULE_VERSION

/* fixup functions */
static int fixup_hostport2proxy(void** param, int param_no);


/* init functions */
static int mod_init(void);
static int child_init(int rank);


/* exported functions */
inline static int w_t_check(struct sip_msg* msg, char* str, char* str2);
inline static int w_t_reply(struct sip_msg* msg, char* str, char* str2);
inline static int w_t_release(struct sip_msg* msg, char* str, char* str2);
inline static int w_t_retransmit_reply(struct sip_msg* p_msg, char* foo,
				char* bar );
inline static int w_t_newtran(struct sip_msg* p_msg, char* foo, char* bar );
inline static int w_t_relay( struct sip_msg  *p_msg , char *_foo, char *_bar);
inline static int w_t_relay_to_udp( struct sip_msg  *p_msg , char *proxy,
				 char *);
#ifdef USE_TCP
inline static int w_t_relay_to_tcp( struct sip_msg  *p_msg , char *proxy,
				char *);
#endif
#ifdef USE_TLS
inline static int w_t_relay_to_tls( struct sip_msg  *p_msg , char *proxy,
				char *);
#endif
inline static int w_t_replicate( struct sip_msg  *p_msg ,
				char *proxy, /* struct proxy_l *proxy expected */
				char *_foo       /* nothing expected */ );
inline static int w_t_replicate_udp( struct sip_msg  *p_msg ,
				char *proxy, /* struct proxy_l *proxy expected */
				char *_foo       /* nothing expected */ );
#ifdef USE_TCP
inline static int w_t_replicate_tcp( struct sip_msg  *p_msg ,
				char *proxy, /* struct proxy_l *proxy expected */
				char *_foo       /* nothing expected */ );
#endif
#ifdef USE_TLS
inline static int w_t_replicate_tls( struct sip_msg  *p_msg ,
				char *proxy, /* struct proxy_l *proxy expected */
				char *_foo       /* nothing expected */ );
#endif
inline static int w_t_forward_nonack(struct sip_msg* msg, char* str, char* );
inline static int w_t_forward_nonack_uri(struct sip_msg* msg, char* str,char*);
inline static int w_t_forward_nonack_udp(struct sip_msg* msg, char* str,char*);
#ifdef USE_TCP
inline static int w_t_forward_nonack_tcp(struct sip_msg* msg, char* str,char*);
#endif
#ifdef USE_TLS
inline static int w_t_forward_nonack_tls(struct sip_msg* msg, char* str,char*);
#endif
inline static int w_t_on_negative(struct sip_msg* msg, char *go_to, char *foo);
inline static int w_t_on_branch(struct sip_msg* msg, char *go_to, char *foo);
inline static int w_t_on_reply(struct sip_msg* msg, char *go_to, char *foo );
inline static int t_check_status(struct sip_msg* msg, char *regexp, char *foo);
static int t_set_fr_inv(struct sip_msg* msg, char* fr_inv, char* foo);
static int t_set_fr_all(struct sip_msg* msg, char* fr_inv, char* fr);


static char *fr_timer_param = FR_TIMER_AVP;
static char *fr_inv_timer_param = FR_INV_TIMER_AVP;

static rpc_export_t tm_rpc[];

static cmd_export_t cmds[]={
	{"t_newtran",          w_t_newtran,             0, 0,
			REQUEST_ROUTE},
	{"t_lookup_request",   w_t_check,               0, 0,
			REQUEST_ROUTE},
	{T_REPLY,              w_t_reply,               2, fixup_int_1,
			REQUEST_ROUTE | FAILURE_ROUTE },
	{"t_retransmit_reply", w_t_retransmit_reply,    0, 0,
			REQUEST_ROUTE},
	{"t_release",          w_t_release,             0, 0,
			REQUEST_ROUTE},
	{T_RELAY_TO_UDP,       w_t_relay_to_udp,        2, fixup_hostport2proxy,
			REQUEST_ROUTE|FAILURE_ROUTE},
#ifdef USE_TCP
	{T_RELAY_TO_TCP,       w_t_relay_to_tcp,        2, fixup_hostport2proxy,
			REQUEST_ROUTE|FAILURE_ROUTE},
#endif
#ifdef USE_TLS
	{T_RELAY_TO_TLS,       w_t_relay_to_tls,        2, fixup_hostport2proxy,
			REQUEST_ROUTE|FAILURE_ROUTE},
#endif
	{"t_replicate",        w_t_replicate,           2, fixup_hostport2proxy,
			REQUEST_ROUTE},
	{"t_replicate_udp",    w_t_replicate_udp,       2, fixup_hostport2proxy,
			REQUEST_ROUTE},
#ifdef USE_TCP
	{"t_replicate_tcp",    w_t_replicate_tcp,       2, fixup_hostport2proxy,
			REQUEST_ROUTE},
#endif
#ifdef USE_TLS
	{"t_replicate_tls",    w_t_replicate_tls,       2, fixup_hostport2proxy,
			REQUEST_ROUTE},
#endif
	{T_RELAY,              w_t_relay,               0, 0,
			REQUEST_ROUTE | FAILURE_ROUTE },
	{T_FORWARD_NONACK,     w_t_forward_nonack,      2, fixup_hostport2proxy,
			REQUEST_ROUTE},
	{T_FORWARD_NONACK_URI, w_t_forward_nonack_uri,  0, 0,
			REQUEST_ROUTE},
	{T_FORWARD_NONACK_UDP, w_t_forward_nonack_udp,  2, fixup_hostport2proxy,
			REQUEST_ROUTE},
#ifdef USE_TCP
	{T_FORWARD_NONACK_TCP, w_t_forward_nonack_tcp,  2, fixup_hostport2proxy,
			REQUEST_ROUTE},
#endif
#ifdef USE_TLS
	{T_FORWARD_NONACK_TLS, w_t_forward_nonack_tls,  2, fixup_hostport2proxy,
			REQUEST_ROUTE},
#endif
	{"t_on_failure",       w_t_on_negative,         1, fixup_int_1,
			REQUEST_ROUTE | FAILURE_ROUTE | ONREPLY_ROUTE },
	{"t_on_reply",         w_t_on_reply,            1, fixup_int_1,
			REQUEST_ROUTE | FAILURE_ROUTE | ONREPLY_ROUTE },
	{"t_on_branch",       w_t_on_branch,         1, fixup_int_1,
			REQUEST_ROUTE | FAILURE_ROUTE },
	{"t_check_status",     t_check_status,          1, fixup_regex_1,
			REQUEST_ROUTE | FAILURE_ROUTE | ONREPLY_ROUTE },
	{"t_write_req",       t_write_req,              2, fixup_t_write,
			REQUEST_ROUTE | FAILURE_ROUTE },
	{"t_write_unix",      t_write_unix,             2, fixup_t_write,
	                REQUEST_ROUTE | FAILURE_ROUTE },
	{"t_set_fr",          t_set_fr_inv,             1, fixup_int_1,
			REQUEST_ROUTE|ONREPLY_ROUTE|FAILURE_ROUTE|BRANCH_ROUTE },
	{"t_set_fr",          t_set_fr_all,             2, fixup_int_12,
			REQUEST_ROUTE|ONREPLY_ROUTE|FAILURE_ROUTE|BRANCH_ROUTE },

	/* not applicable from the script */
	{"register_tmcb",      (cmd_function)register_tmcb,     NO_SCRIPT,   0, 0},
	{"load_tm",            (cmd_function)load_tm,           NO_SCRIPT,   0, 0},
	{T_REPLY_WB,           (cmd_function)t_reply_with_body, NO_SCRIPT,   0, 0},
	{T_IS_LOCAL,           (cmd_function)t_is_local,        NO_SCRIPT,   0, 0},
	{T_GET_TI,             (cmd_function)t_get_trans_ident, NO_SCRIPT,   0, 0},
	{T_LOOKUP_IDENT,       (cmd_function)t_lookup_ident,    NO_SCRIPT,   0, 0},
	{T_ADDBLIND,           (cmd_function)add_blind_uac,     NO_SCRIPT,   0, 0},
	{"t_request_within",   (cmd_function)req_within,        NO_SCRIPT,   0, 0},
	{"t_request_outside",  (cmd_function)req_outside,       NO_SCRIPT,   0, 0},
	{"t_request",          (cmd_function)request,           NO_SCRIPT,   0, 0},
	{"new_dlg_uac",        (cmd_function)new_dlg_uac,       NO_SCRIPT,   0, 0},
	{"dlg_response_uac",   (cmd_function)dlg_response_uac,  NO_SCRIPT,   0, 0},
	{"new_dlg_uas",        (cmd_function)new_dlg_uas,       NO_SCRIPT,   0, 0},
	{"dlg_request_uas",    (cmd_function)dlg_request_uas,   NO_SCRIPT,   0, 0},
	{"free_dlg",           (cmd_function)free_dlg,          NO_SCRIPT,   0, 0},
	{"print_dlg",          (cmd_function)print_dlg,         NO_SCRIPT,   0, 0},
	{T_GETT,               (cmd_function)get_t,             NO_SCRIPT,   0, 0},
	{"calculate_hooks",    (cmd_function)w_calculate_hooks, NO_SCRIPT,   0, 0},
	{0,0,0,0,0}
};


static param_export_t params[]={
	{"ruri_matching",       PARAM_INT, &ruri_matching                        },
	{"via1_matching",       PARAM_INT, &via1_matching                        },
	{"fr_timer",            PARAM_INT, &fr_timeout                           },
	{"fr_inv_timer",        PARAM_INT, &fr_inv_timeout                       },
	{"wt_timer",            PARAM_INT, &wait_timeout                         },
	{"delete_timer",        PARAM_INT, &delete_timeout                       },
	{"retr_timer1",         PARAM_INT, &rt_t1_timeout                        },
	{"retr_timer2"  ,       PARAM_INT, &rt_t2_timeout                        },
	{"noisy_ctimer",        PARAM_INT, &noisy_ctimer                         },
	{"uac_from",            PARAM_STRING, &uac_from                          },
	{"unix_tx_timeout",     PARAM_INT, &tm_unix_tx_timeout                   },
	{"restart_fr_on_each_reply", PARAM_INT, &restart_fr_on_each_reply        },
	{"fr_timer_avp",        PARAM_STRING, &fr_timer_param                    },
	{"fr_inv_timer_avp",    PARAM_STRING, &fr_inv_timer_param                },
	{"tw_append",           PARAM_STRING|PARAM_USE_FUNC, (void*)parse_tw_append },
        {"pass_provisional_replies", PARAM_INT, &pass_provisional_replies        },
	{0,0,0}
};


#ifdef STATIC_TM
struct module_exports tm_exports = {
#else
struct module_exports exports= {
#endif
	"tm",
	/* -------- exported functions ----------- */
	cmds,
	tm_rpc,    /* RPC methods */
	/* ------------ exported variables ---------- */
	params,

	mod_init, /* module initialization function */
	(response_function) reply_received,
	(destroy_function) tm_shutdown,
	0, /* w_onbreak, */
	child_init /* per-child init function */
};


/* (char *hostname, char *port_nr) ==> (struct proxy_l *, -)  */
static int fixup_hostport2proxy(void** param, int param_no)
{
	unsigned int port;
	char *host;
	int err;
	struct proxy_l *proxy;
	action_u_t *a;
	str s;

	DBG("TM module: fixup_hostport2proxy(%s, %d)\n", (char*)*param, param_no);
	if (param_no==1){
		return 0;
	} else if (param_no==2) {
		a = fixup_get_param(param, param_no, 1);
		host= a->u.string;
		port=str2s(*param, strlen(*param), &err);
		if (err!=0) {
			LOG(L_ERR, "TM module:fixup_hostport2proxy: bad port number <%s>\n",
				(char*)(*param));
			 return E_UNSPEC;
		}
		s.s = host;
		s.len = strlen(host);
		proxy=mk_proxy(&s, port, 0); /* FIXME: udp or tcp? */
		if (proxy==0) {
			LOG(L_ERR, "ERROR: fixup_hostport2proxy: bad host name in URI <%s>\n",
				host );
			return E_BAD_ADDRESS;
		}
		/* success -- fix the first parameter to proxy now ! */

		a->u.data=proxy;
		return 0;
	} else {
		LOG(L_ERR,"ERROR: fixup_hostport2proxy called with parameter #<>{1,2}\n");
		return E_BUG;
	}
}


/***************************** init functions *****************************/
static int w_t_unref( struct sip_msg *foo, void *bar)
{
	return t_unref(foo);
}


static int script_init( struct sip_msg *foo, void *bar)
{
	/* we primarily reset all private memory here to make sure
	 * private values left over from previous message will
	 * not be used again */

	/* make sure the new message will not inherit previous
		message's t_on_negative value
	*/
	t_on_negative( 0 );
	t_on_reply(0);
	t_on_branch(0);
	/* reset the kr status */
	set_kr(0);
	/* set request mode so that multiple-mode actions know
	 * how to behave */
	rmode=MODE_REQUEST;
	return 1;
}


static int mod_init(void)
{
	DBG( "TM - (sizeof cell=%ld, sip_msg=%ld) initializing...\n",
			(long)sizeof(struct cell), (long)sizeof(struct sip_msg));
	/* checking if we have sufficient bitmap capacity for given
	   maximum number of  branches */
	if (MAX_BRANCHES+1>31) {
		LOG(L_CRIT, "Too many max UACs for UAC branch_bm_t bitmap: %d\n",
			MAX_BRANCHES );
		return -1;
	}

	if (init_callid() < 0) {
		LOG(L_CRIT, "Error while initializing Call-ID generator\n");
		return -1;
	}

	/* building the hash table*/
	if (!init_hash_table()) {
		LOG(L_ERR, "ERROR: mod_init: initializing hash_table failed\n");
		return -1;
	}

	/* init static hidden values */
	init_t();

	if (tm_init_timers()==-1) {
		LOG(L_ERR, "ERROR: mod_init: timer init failed\n");
		return -1;
	}

	     /* First tm_stat initialization function only allocates the top level stat
	      * structure in shared memory, the initialization will complete in child
	      * init with init_tm_stats_child when the final value of process_count is
	      * known
	      */
	if (init_tm_stats() < 0) {
		LOG(L_CRIT, "ERROR: mod_init: failed to init stats\n");
		return -1;
	}

	if (uac_init()==-1) {
		LOG(L_ERR, "ERROR: mod_init: uac_init failed\n");
		return -1;
	}

	if (init_tmcb_lists()!=1) {
		LOG(L_CRIT, "ERROR:tm:mod_init: failed to init tmcb lists\n");
		return -1;
	}

	tm_init_tags();
	init_twrite_lines();
	if (init_twrite_sock() < 0) {
		LOG(L_ERR, "ERROR:tm:mod_init: Unable to create socket\n");
		return -1;
	}

	/* register post-script clean-up function */
	if (register_script_cb( w_t_unref, POST_SCRIPT_CB|REQ_TYPE_CB, 0)<0 ) {
		LOG(L_ERR,"ERROR:tm:mod_init: failed to register POST request "
			"callback\n");
		return -1;
	}
	if (register_script_cb( script_init, PRE_SCRIPT_CB|REQ_TYPE_CB , 0)<0 ) {
		LOG(L_ERR,"ERROR:tm:mod_init: failed to register PRE request "
			"callback\n");
		return -1;
	}

	if (init_avp_params( fr_timer_param, fr_inv_timer_param)<0 ){
		LOG(L_ERR,"ERROR:tm:mod_init: failed to process timer AVPs\n");
		return -1;
	}

	tm_init = 1;
	return 0;
}

static int child_init(int rank) {
	if (child_init_callid(rank) < 0) {
		LOG(L_ERR, "ERROR: child_init: Error while initializing Call-ID generator\n");
		return -2;
	}

	if (rank == PROC_MAIN) {
		if (init_tm_stats_child() < 0) {
			ERR("Error while initializing tm statistics structures\n");
			return -1;
		}
	}

	return 0;
}





/**************************** wrapper functions ***************************/
static int t_check_status(struct sip_msg* msg, char *regexp, char *foo)
{
	regmatch_t pmatch;
	struct cell *t;
	char *status;
	char backup;
	int lowest_status;
	int n;

	/* first get the transaction */
	if (t_check( msg , 0 )==-1) return -1;
	if ( (t=get_t())==0) {
		LOG(L_ERR, "ERROR: t_check_status: cannot check status for a reply "
			"which has no T-state established\n");
		return -1;
	}
	backup = 0;

	switch (rmode) {
		case MODE_REQUEST:
			/* use the status of the last sent reply */
			status = int2str( t->uas.status, 0);
			break;
		case MODE_ONREPLY:
			/* use the status of the current reply */
			status = msg->first_line.u.reply.status.s;
			backup = status[msg->first_line.u.reply.status.len];
			status[msg->first_line.u.reply.status.len] = 0;
			break;
		case MODE_ONFAILURE:
			/* use the status of the winning reply */
			if (t_pick_branch( -1, 0, t, &lowest_status)<0 ) {
				LOG(L_CRIT,"BUG:t_check_status: t_pick_branch failed to get "
					" a final response in MODE_ONFAILURE\n");
				return -1;
			}
			status = int2str( lowest_status , 0);
			break;
		default:
			LOG(L_ERR,"ERROR:t_check_status: unsupported mode %d\n",rmode);
			return -1;
	}

	DBG("DEBUG:t_check_status: checked status is <%s>\n",status);
	/* do the checking */
	n = regexec((regex_t*)regexp, status, 1, &pmatch, 0);

	if (backup) status[msg->first_line.u.reply.status.len] = backup;
	if (n!=0) return -1;
	return 1;
}


inline static int w_t_check(struct sip_msg* msg, char* str, char* str2)
{
	return t_check( msg , 0  ) ? 1 : -1;
}


inline static int _w_t_forward_nonack(struct sip_msg* msg, char* proxy,
																	int proto)
{
	struct cell *t;
	if (t_check( msg , 0 )==-1) {
		LOG(L_ERR, "ERROR: forward_nonack: "
				"can't forward when no transaction was set up\n");
		return -1;
	}
	t=get_t();
	if ( t && t!=T_UNDEFINED ) {
		if (msg->REQ_METHOD==METHOD_ACK) {
			LOG(L_WARN,"WARNING: you don't really want to fwd hbh ACK\n");
			return -1;
		}
		return t_forward_nonack(t, msg, ( struct proxy_l *) proxy, proto );
	} else {
		DBG("DEBUG: forward_nonack: no transaction found\n");
		return -1;
	}
}


inline static int w_t_forward_nonack( struct sip_msg* msg, char* proxy,
										char* foo)
{
	return _w_t_forward_nonack(msg, proxy, PROTO_NONE);
}


inline static int w_t_forward_nonack_uri(struct sip_msg* msg, char *foo,
																	char *bar)
{
	return _w_t_forward_nonack(msg, 0, PROTO_NONE);
}


inline static int w_t_forward_nonack_udp( struct sip_msg* msg, char* proxy,
										char* foo)
{
	return _w_t_forward_nonack(msg, proxy, PROTO_UDP);
}


#ifdef USE_TCP
inline static int w_t_forward_nonack_tcp( struct sip_msg* msg, char* proxy,
										char* foo)
{
	return _w_t_forward_nonack(msg, proxy, PROTO_TCP);
}
#endif


#ifdef USE_TLS
inline static int w_t_forward_nonack_tls( struct sip_msg* msg, char* proxy,
										char* foo)
{
	return _w_t_forward_nonack(msg, proxy, PROTO_TLS);
}
#endif


inline static int w_t_reply(struct sip_msg* msg, char* str, char* str2)
{
	struct cell *t;

	if (msg->REQ_METHOD==METHOD_ACK) {
		LOG(L_WARN, "WARNING: t_reply: ACKs are not replied\n");
		return -1;
	}
	if (t_check( msg , 0 )==-1) return -1;
	t=get_t();
	if (!t) {
		LOG(L_ERR, "ERROR: t_reply: cannot send a t_reply to a message "
			"for which no T-state has been established\n");
		return -1;
	}
	/* if called from reply_route, make sure that unsafe version
	 * is called; we are already in a mutex and another mutex in
	 * the safe version would lead to a deadlock
	 */
	if (rmode==MODE_ONFAILURE) {
		DBG("DEBUG: t_reply_unsafe called from w_t_reply\n");
		return t_reply_unsafe(t, msg, (unsigned int)(long) str, str2);
	} else if (rmode==MODE_REQUEST) {
		return t_reply( t, msg, (unsigned int)(long) str, str2);
	} else {
		LOG(L_CRIT, "BUG: w_t_reply entered in unsupported mode\n");
		return -1;
	}
}


inline static int w_t_release(struct sip_msg* msg, char* str, char* str2)
{
	struct cell *t;
	if (t_check( msg  , 0  )==-1) return -1;
	t=get_t();
	if ( t && t!=T_UNDEFINED )
		return t_release_transaction( t );
	return 1;
}


inline static int w_t_retransmit_reply( struct sip_msg* p_msg, char* foo, char* bar)
{
	struct cell *t;


	if (t_check( p_msg  , 0 )==-1)
		return 1;
	t=get_t();
	if (t) {
		if (p_msg->REQ_METHOD==METHOD_ACK) {
			LOG(L_WARN, "WARNING: : ACKs transmit_replies not replied\n");
			return -1;
		}
		return t_retransmit_reply( t );
	} else
		return -1;
}


inline static int w_t_newtran( struct sip_msg* p_msg, char* foo, char* bar )
{
	/* t_newtran returns 0 on error (negative value means
	   'transaction exists' */
	return t_newtran( p_msg );
}


inline static int w_t_on_negative( struct sip_msg* msg, char *go_to, char *foo)
{
	t_on_negative( (unsigned int )(long) go_to );
	return 1;
}

inline static int w_t_on_branch( struct sip_msg* msg, char *go_to, char *foo)
{
	t_on_branch( (unsigned int )(long) go_to );
	return 1;
}


inline static int w_t_on_reply( struct sip_msg* msg, char *go_to, char *foo )
{
	t_on_reply( (unsigned int )(long) go_to );
	return 1;
}



inline static int _w_t_relay_to( struct sip_msg  *p_msg ,
	struct proxy_l *proxy )
{
	struct cell *t;

	if (rmode==MODE_ONFAILURE) {
		t=get_t();
		if (!t || t==T_UNDEFINED) {
			LOG(L_CRIT, "BUG: w_t_relay_to: undefined T\n");
			return -1;
		}
		if (t_forward_nonack(t, p_msg, proxy, PROTO_NONE)<=0 ) {
			LOG(L_ERR, "ERROR: w_t_relay_to: t_relay_to failed\n");
			return -1;
		}
		return 1;
	}
	if (rmode==MODE_REQUEST)
		return t_relay_to( p_msg, proxy, PROTO_NONE,
			0 /* no replication */ );
	LOG(L_CRIT, "ERROR: w_t_relay_to: unsupported mode: %d\n", rmode);
	return 0;
}


inline static int w_t_relay_to_udp( struct sip_msg  *p_msg ,
	char *proxy, /* struct proxy_l *proxy expected */
	char *_foo       /* nothing expected */ )
{
	((struct proxy_l *)proxy)->proto=PROTO_UDP;
	return _w_t_relay_to( p_msg, ( struct proxy_l *) proxy);
}


#ifdef USE_TCP
inline static int w_t_relay_to_tcp( struct sip_msg  *p_msg ,
	char *proxy, /* struct proxy_l *proxy expected */
	char *_foo       /* nothing expected */ )
{
	((struct proxy_l *)proxy)->proto=PROTO_TCP;
	return _w_t_relay_to( p_msg, ( struct proxy_l *) proxy);
}
#endif


#ifdef USE_TLS
inline static int w_t_relay_to_tls( struct sip_msg  *p_msg ,
	char *proxy, /* struct proxy_l *proxy expected */
	char *_foo       /* nothing expected */ )
{
	((struct proxy_l *)proxy)->proto=PROTO_TLS;
	return _w_t_relay_to( p_msg, ( struct proxy_l *) proxy);
}
#endif


inline static int w_t_replicate( struct sip_msg  *p_msg ,
	char *proxy, /* struct proxy_l *proxy expected */
	char *_foo       /* nothing expected */ )
{
	return t_replicate(p_msg, ( struct proxy_l *) proxy, p_msg->rcv.proto );
}


inline static int w_t_replicate_udp( struct sip_msg  *p_msg ,
	char *proxy, /* struct proxy_l *proxy expected */
	char *_foo       /* nothing expected */ )
{
	return t_replicate(p_msg, ( struct proxy_l *) proxy, PROTO_UDP );
}


#ifdef USE_TCP
inline static int w_t_replicate_tcp( struct sip_msg  *p_msg ,
	char *proxy, /* struct proxy_l *proxy expected */
	char *_foo       /* nothing expected */ )
{
	return t_replicate(p_msg, ( struct proxy_l *) proxy, PROTO_TCP );
}
#endif


#ifdef USE_TLS
inline static int w_t_replicate_tls( struct sip_msg  *p_msg ,
	char *proxy, /* struct proxy_l *proxy expected */
	char *_foo       /* nothing expected */ )
{
	return t_replicate(p_msg, ( struct proxy_l *) proxy, PROTO_TLS );
}
#endif


inline static int w_t_relay( struct sip_msg  *p_msg ,
						char *_foo, char *_bar)
{
	struct cell *t;

	if (rmode==MODE_ONFAILURE) {
		t=get_t();
		if (!t || t==T_UNDEFINED) {
			LOG(L_CRIT, "BUG: w_t_relay: undefined T\n");
			return -1;
		}
		if (t_forward_nonack(t, p_msg, ( struct proxy_l *) 0, PROTO_NONE)<=0) {
			LOG(L_ERR, "ERROR: w_t_relay (failure mode): forwarding failed\n");
			return -1;
		}
		return 1;
	}
	if (rmode==MODE_REQUEST)
		return t_relay_to( p_msg,
		(struct proxy_l *) 0 /* no proxy */, PROTO_NONE,
		0 /* no replication */ );
	LOG(L_CRIT, "ERROR: w_t_relay_to: unsupported mode: %d\n", rmode);
	return 0;
}


/* set fr_inv_timeout & or fr_timeout; 0 means: use the default value */
static int t_set_fr_all(struct sip_msg* msg, char* fr_inv, char* fr)
{

	return t_set_fr(msg, (unsigned int)(long)fr_inv, (unsigned int)(long)fr);
}

static int t_set_fr_inv(struct sip_msg* msg, char* fr_inv, char* foo)
{
	return t_set_fr_all(msg, fr_inv, (char*)0);
}


static rpc_export_t tm_rpc[] = {
	{"tm.cancel", rpc_cancel,   rpc_cancel_doc,   0},
	{"tm.reply",  rpc_reply,    rpc_reply_doc,    0},
	{"tm.stats",  tm_rpc_stats, tm_rpc_stats_doc, 0},
	{0, 0, 0, 0}
};
