/* 
 * $Id: auth_gsm.c,v 1.1 2003/12/09 12:43:22 dcm Exp $ 
 *
 * GSM Authentication
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
 *
 * History:
 * -------
 *  2003-07-03: first version (dcm)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../sr_module.h"
#include "../../error.h"
#include "../../dprint.h"
#include "../../mem/mem.h"
#include "auth_gsm.h"
#include "authorize.h"
#include <radiusclient.h>
#include "gsm_radius.h"

MODULE_VERSION

pre_auth_f pre_auth_func = 0;   /* Pre authorization function from auth module */
post_auth_f post_auth_func = 0; /* Post authorization function from auth module */

/*
 * Pointer to reply function in stateless module
 */
int (*sl_reply)(struct sip_msg* _msg, char* _str1, char* _str2);

/* Module initialization function */
static int mod_init(void);
/* char* -> str* */
static int str_fixup(void** param, int param_no);


/*
 * Module parameter variables
 */
char* radius_config = "/usr/local/etc/radiusclient/radiusclient.conf";
int service_type = PW_GSM_AUTH;


/*
 * Exported functions
 */
static cmd_export_t cmds[] = {
	{"gsm_www_authorize",   gsm_www_authorize,   1, str_fixup, REQUEST_ROUTE},
	{"gsm_proxy_authorize", gsm_proxy_authorize, 1, str_fixup, REQUEST_ROUTE},
	{0, 0, 0, 0, 0}
};


/*
 * Exported parameters
 */
static param_export_t params[] = {
	{"radius_config", STR_PARAM, &radius_config},
	{"service_type",  INT_PARAM, &service_type },
	{0, 0, 0}
};


/*
 * Module interface
 */
struct module_exports exports = {
	"auth_gsm", 
	cmds,       /* Exported functions */
	params,     /* Exported parameters */
	mod_init,   /* module initialization function */
	0,          /* response function */
	0,          /* destroy function */
	0,          /* oncancel function */
	0           /* child initialization function */
};


/*
 * Module initialization function
 */
static int mod_init(void)
{
	DBG("auth_gsm - Initializing\n");

	if (rc_read_config(radius_config) != 0) {
		LOG(L_ERR, "auth_gsm: Error opening radius configuration file \n");
		return -1;
	}
    
	if (rc_read_dictionary(rc_conf_str("dictionary")) != 0) {
		LOG(L_ERR, "auth_gsm: Error opening radius dictionary file \n");
		return -2;
	}
	
	sl_reply = find_export("sl_send_reply", 2, 0);

	if (!sl_reply) 
	{
		LOG(L_ERR, "auth_gsm:mod_init: This module requires sl module\n");
		return -2;
	}

	pre_auth_func = (pre_auth_f)find_export("pre_auth", 0, 0);
	post_auth_func = (post_auth_f)find_export("post_auth", 0, 0);

	if (!(pre_auth_func && post_auth_func)) {
		LOG(L_ERR, "auth_gsm:mod_init: This module requires auth module\n");
		return -3;
	}

	return 0;
}


/*
 * Convert char* parameter to str* parameter
 */
static int str_fixup(void** param, int param_no)
{
	str* s;

	if (param_no == 1) {
		s = (str*)pkg_malloc(sizeof(str));
		if (!s) {
			LOG(L_ERR, "auth_gsm:str_fixup: No memory left\n");
			return E_UNSPEC;
		}

		s->s = (char*)*param;
		s->len = strlen(s->s);
		*param = (void*)s;
	}

	return 0;
}
