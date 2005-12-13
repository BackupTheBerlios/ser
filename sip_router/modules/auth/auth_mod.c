/* 
 * $Id: auth_mod.c,v 1.60 2005/12/13 00:19:38 janakj Exp $ 
 *
 * Digest Authentication Module
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
 * 2003-02-26 checks and group moved to separate modules (janakj)
 * 2003-03-10 New module interface (janakj)
 * 2003-03-16 flags export parameter added (janakj)
 * 2003-03-19 all mallocs/frees replaced w/ pkg_malloc/pkg_free (andrei)
 * 2003-04-28 rpid contributed by Juha Heinanen added (janakj) 
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../mem/mem.h"
#include "../../error.h"
#include "../../ut.h"
#include "auth_mod.h"
#include "challenge.h"
#include "api.h"

MODULE_VERSION

#define RAND_SECRET_LEN 32


/*
 * Module destroy function prototype
 */
static void destroy(void);

/*
 * Module initialization function prototype
 */
static int mod_init(void);


static int challenge_fixup(void** param, int param_no);


/*
 * Pointer to reply function in stateless module
 */
int (*sl_reply)(struct sip_msg* msg, char* str1, char* str2);


/*
 * Module parameter variables
 */
char* sec_param    = 0;     /* If the parameter was not used, the secret phrase will be auto-generated */
int   nonce_expire = 300;   /* Nonce lifetime */
int   protect_contacts = 0; /* Do not include contacts in nonce by default */

str secret;
char* sec_rand = 0;


/*
 * Exported functions 
 */
static cmd_export_t cmds[] = {
	{"www_challenge",       www_challenge,           2, challenge_fixup, REQUEST_ROUTE},
	{"proxy_challenge",     proxy_challenge,         2, challenge_fixup, REQUEST_ROUTE},
	{"consume_credentials", consume_credentials,     0, 0,               REQUEST_ROUTE},
	{"bind_auth",           (cmd_function)bind_auth, 0, 0,               0            },
	{0, 0, 0, 0, 0}
};


/*
 * Exported parameters
 */
static param_export_t params[] = {
	{"secret",           STR_PARAM, &sec_param       },
	{"nonce_expire",     INT_PARAM, &nonce_expire    },
	{"protect_contacts", INT_PARAM, &protect_contacts},
	{0, 0, 0}
};


/*
 * Module interface
 */
struct module_exports exports = {
	"auth", 
	cmds,
	0,          /* RPC methods */
	params,
	mod_init,   /* module initialization function */
	0,          /* response function */
	destroy,    /* destroy function */
	0,          /* oncancel function */
	0           /* child initialization function */
};


/*
 * Secret parameter was not used so we generate
 * a random value here
 */
static inline int generate_random_secret(void)
{
	int i;

	sec_rand = (char*)pkg_malloc(RAND_SECRET_LEN);
	if (!sec_rand) {
		LOG(L_ERR, "auth:generate_random_secret: No memory left\n");
		return -1;
	}

	srandom(time(0));

	for(i = 0; i < RAND_SECRET_LEN; i++) {
		sec_rand[i] = 32 + (int)(95.0 * rand() / (RAND_MAX + 1.0));
	}

	secret.s = sec_rand;
	secret.len = RAND_SECRET_LEN;

	     /*	DBG("Generated secret: '%.*s'\n", secret.len, secret.s); */

	return 0;
}


static int mod_init(void)
{
	DBG("auth module - initializing\n");
	
	sl_reply = find_export("sl_send_reply", 2, 0);

	if (!sl_reply) {
		LOG(L_ERR, "auth:mod_init: This module requires sl module\n");
		return -2;
	}

	     /* If the parameter was not used */
	if (sec_param == 0) {
		     /* Generate secret using random generator */
		if (generate_random_secret() < 0) {
			LOG(L_ERR, "auth:mod_init: Error while generating random secret\n");
			return -3;
		}
	} else {
		     /* Otherwise use the parameter's value */
		secret.s = sec_param;
		secret.len = strlen(secret.s);
	}
	
	return 0;
}



static void destroy(void)
{
	if (sec_rand) pkg_free(sec_rand);
}


static int challenge_fixup(void** param, int param_no)
{
	if (param_no == 1) {
		return fixup_str_12(param, param_no);
	} else if (param_no == 2) {
		return fixup_int_12(param, param_no);
	}

	return 0;
}
