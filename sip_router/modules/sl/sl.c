/*
 * $Id: sl.c,v 1.10 2002/10/03 20:06:10 jiri Exp $
 *
 * sl module
 *
 *
 * ************************************************ *
 * * Bogdan's Source Memorial                       *
 * *                                                *
 * * Welcome, pilgrame! This is one of rare places  *
 * * kept untouched in memory of brave heart,       *
 * * Bogdan, one of most active ser contributors,   *
 * * and winner of the longest line of code content.*
 * *                                                *
 * * Please, preserve this codework heritage, as    *
 * * most of other work has been smashed away during*
 * * extensive clean-up floods.                     *
 * *                                                *
 * * Hereby, we solicit you to adopt this historical*
 * * piece of code. For $100, your name will be     *
 * * be printed in this banner and we will use      *
 * * collected funds to create and display an ASCII *
 * * statue of Bogdan.                              *
 * **************************************************
 *
 *
 * Copyright (C) 2001-2003 Fhg Fokus
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../error.h"
#include "../../ut.h"
#include "../../script_cb.h"
#include "sl_stats.h"
#include "sl_funcs.h"


#ifdef _OBSOLETED
static int w_sl_filter_ACK(struct sip_msg* msg, char* str, char* str2);
#endif
static int w_sl_send_reply(struct sip_msg* msg, char* str, char* str2);
static int w_sl_reply_error(struct sip_msg* msg, char* str, char* str2);
static int fixup_sl_send_reply(void** param, int param_no);
static int mod_init(void);
static int mod_destroy();

#ifdef STATIC_SL
struct module_exports sl_exports = {
#else
struct module_exports exports= {
#endif
	"sl_module",
	(char*[]){
				"sl_send_reply",
#ifdef _OBSOLETED
				"sl_filter_ACK",
#endif
				"sl_reply_error"
			},
	(cmd_function[]){
					w_sl_send_reply,
#ifdef _OBSOLETED
					w_sl_filter_ACK,
#endif
					w_sl_reply_error
					},
	(int[]){
				2,
#ifdef _OBSOLETED
				0,
#endif
				0
			},
	(fixup_function[]){
				fixup_sl_send_reply,
#ifdef _OBSOLETED
				0, /* sl_filter_ACK */
#endif
				0 /* sl_reply_error */
		},
	/* 3, */ 2,

	NULL,   /* Module parameter names */
	NULL,   /* Module parameter types */
	NULL,   /* Module parameter variable pointers */
	0,      /* Number of module paramers */

	mod_init,   /* module initialization function */
	(response_function) 0,
	(destroy_function) sl_shutdown,
	0,
	0  /* per-child init function */
};




static int mod_init(void)
{
	fprintf(stderr, "stateless - initializing\n");
	if (init_sl_stats()<0) {
		LOG(L_ERR, "ERROR: init_sl_stats failed\n");
		return -1;
	}
	/* if SL loaded, filter ACKs on beginning */
	register_script_cb( sl_filter_ACK, PRE_SCRIPT_CB, 0 );
	sl_startup();

	return 0;
}




static int mod_destroy()
{
	sl_stats_destroy();
	return sl_shutdown();
}




static int fixup_sl_send_reply(void** param, int param_no)
{
	unsigned int code;
	int err;

	if (param_no==1){
		code=str2s(*param, strlen(*param), &err);
		if (err==0){
			free(*param);
			*param=(void*)code;
			return 0;
		}else{
			LOG(L_ERR, "SL module:fixup_sl_send_reply: bad  number <%s>\n",
					(char*)(*param));
			return E_UNSPEC;
		}
	}
	return 0;
}




#ifdef _OBSOLETED
static int w_sl_filter_ACK(struct sip_msg* msg, char* str, char* str2)
{
	return sl_filter_ACK(msg);
}
#endif



static int w_sl_send_reply(struct sip_msg* msg, char* str, char* str2)
{
	return sl_send_reply(msg,(unsigned int)str,str2);
}


static int w_sl_reply_error( struct sip_msg* msg, char* str, char* str2)
{
	return sl_reply_error( msg );
}


