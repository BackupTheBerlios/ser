/*
 * Presence Agent, module interface
 *
 * $Id: pa_mod.c,v 1.1 2002/11/14 14:29:48 janakj Exp $
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


#include "pa_mod.h"
#include "../../sr_module.h"
#include <stdio.h>
#include "subscribe.h"
#include "dlist.h"
#include "../../error.h"


static int mod_init(void);
static void destroy(void);
static int subscribe_fixup(void** param, int param_no);

int default_expires = 3600;

/** TM bind */
struct tm_binds tmb;

/*
 * sl_send_reply function pointer
 */
int (*sl_reply)(struct sip_msg* _m, char* _s1, char* _s2);


int (*ul_register_watcher)(str* _d, str* _a, void* cb, void* data);
int (*ul_unregister_watcher)(str* _d, str* _a, void* cb, void* data);


struct module_exports exports = {
	"pa", 
	(char*[]) {
		"subscribe"
	},
	(cmd_function[]) {
		subscribe
	},
	(int[]) {
		1
	},
	(fixup_function[]) {
		subscribe_fixup
	},
	1, /* number of functions*/

	(char*[]) {
		"default_expires"
	},
	(modparam_t[]) {
		INT_PARAM
	},
	(void*[]) {
		&default_expires
	},
	1,
	
	mod_init, /* module initialization function */
	0,        /* response function*/
	destroy,  /* destroy function */
	0,        /* oncancel function */
	0         /* per-child init function */
};


static int mod_init(void)
{
	load_tm_f load_tm;

	fprintf(stderr, "Presence Agent - initializing\n");

             /*
              * We will need sl_send_reply from stateless
	      * module for sending replies
	      */
        sl_reply = find_export("sl_send_reply", 2);
	if (!sl_reply) {
		LOG(L_ERR, "This module requires sl module\n");
		return -1;
	}

	/* import the TM auto-loading function */
	if ( !(load_tm=(load_tm_f)find_export("load_tm", NO_SCRIPT))) {
		LOG(L_ERR, "Can't import tm\n");
		return -1;
	}
	/* let the auto-loading function load all TM stuff */
	if (load_tm( &tmb )==-1)
		return -1;
	
	(cmd_function)ul_register_watcher = find_export("~ul_register_watcher", 1);
	if (ul_register_watcher == 0) {
		LOG(L_ERR, "Cant find callback\n");
		return -1;
	}

	(cmd_function)ul_unregister_watcher = find_export("~ul_unregister_watcher", 1);
	if (ul_unregister_watcher == 0) {
		LOG(L_ERR, "Cant find callback\n");
		return -1;
	}


	return 0;
}


static void destroy(void)
{
	//	free_all_pdomains();
}


/*
 * Convert char* parameter to udomain_t* pointer
 */
static int subscribe_fixup(void** param, int param_no)
{
	pdomain_t* d;

	if (param_no == 1) {
		if (register_pdomain((char*)*param, &d) < 0) {
			LOG(L_ERR, "subscribe_fixup(): Error while registering domain\n");
			return E_UNSPEC;
		}

		*param = (void*)d;
	}
	return 0;
}
