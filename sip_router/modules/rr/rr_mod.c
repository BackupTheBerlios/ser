/*
 * Route & Record-Route module
 *
 * $Id: rr_mod.c,v 1.36 2005/12/13 00:19:32 janakj Exp $
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
/* History:
 * --------
 *  2003-03-11  updated to the new module interface (andrei)
 *  2003-03-16  flags export parameter added (janakj)
 *  2003-03-19  all mallocs/frees replaced w/ pkg_malloc/pkg_free (andrei)
 *  2003-04-01  Added record_route with ip address parameter (janakj)
 *  2003-04-14  enable_full_lr parameter introduced (janakj)
 */


#include <stdio.h>
#include <stdlib.h>
#include "../../sr_module.h"
#include "../../ut.h"
#include "../../error.h"
#include "../../mem/mem.h"
#include "loose.h"
#include "record.h"
#include "avp_cookie.h"
#include <sys/types.h> /* for regex */
#include <regex.h>
#include "../../script_cb.h"

#ifdef ENABLE_USER_CHECK
#include <string.h>
#include "../../str.h"
str i_user;
char *ignore_user = NULL;
#endif

int append_fromtag = 1;
int enable_double_rr = 1; /* Enable using of 2 RR by default */
int enable_full_lr = 0;   /* Disabled by default */
int add_username = 0;     /* Do not add username by default */

MODULE_VERSION

static int mod_init(void);
static int fixup_avp_regex(void** param, int param_no);


/*
 * Exported functions
 */
/*
 * I do not want people to use strict routing so it is disabled by default,
 * you should always use loose routing, if you really need strict routing then
 * you can replace the last zeroes with REQUEST_ROUTE to enable strict_route and
 * record_route_strict. Don't do that unless you know what you are really doing !
 * Oh, BTW, have I mentioned already that you shouldn't use strict routing ?
 */
static cmd_export_t cmds[] = {
	{"loose_route",          loose_route,         0, 0,           REQUEST_ROUTE},
	{"record_route",         record_route,        0, 0,           REQUEST_ROUTE},
	{"record_route_preset",  record_route_preset, 1, fixup_str_1, REQUEST_ROUTE},
	{"record_route_strict" , record_route_strict, 0, 0,           0            },
	{"rr_add_avp_cookie",    rr_add_avp_cookie,   1, fixup_avp_regex, REQUEST_ROUTE},
	{0, 0, 0, 0, 0}
};


/*
 * Exported parameters
 */
static param_export_t params[] ={
	{"append_fromtag",   INT_PARAM, &append_fromtag  },
	{"enable_double_rr", INT_PARAM, &enable_double_rr},
	{"enable_full_lr",   INT_PARAM, &enable_full_lr  },
#ifdef ENABLE_USER_CHECK
	{"ignore_user",      STR_PARAM, &ignore_user     },
#endif
	{"add_username",     INT_PARAM, &add_username    },
	{0, 0, 0 }
};


struct module_exports exports = {
	"rr",
	cmds,      /* Exported functions */
	0,         /* RPC methods */
	params,    /* Exported parameters */
	mod_init,  /* initialize module */
	0,         /* response function*/
	0,         /* destroy function */
	0,         /* oncancel function */
	0          /* per-child init function */
};


static int mod_init(void)
{
	DBG("rr - initializing\n");
#ifdef ENABLE_USER_CHECK
	if(ignore_user)
	{
		i_user.s = ignore_user;
		i_user.len = strlen(ignore_user);
	}
	else
	{
		i_user.s = 0;
		i_user.len = 0;
	}
#endif
	register_script_cb(rr_before_script_cb, REQ_TYPE_CB | PRE_SCRIPT_CB, 0);
	return 0;
}

static int fixup_avp_regex(void** param, int param_no)
{
	avp_save_item_t* re;

	DBG("rr:fixup_avp_regex: #%d, '%s'\n", param_no, (char*)(*param));
	if (param_no!=1) return 0;
	if ((re=pkg_malloc(sizeof(avp_save_item_t)))==0) return E_OUT_OF_MEM;
	if (strncmp(*param, "s:", 2) == 0) {
		re->type = AVP_NAME_STR;
		re->u.s.s = *param +2;
		re->u.s.len = strlen(re->u.s.s);
	}
	else if (strncmp(*param, "i:", 2) == 0) {
		re->type = 0;
		re->u.n = atol(*param +2);
		if (re->u.n == 0) {
			LOG(L_ERR, "ERROR: %s : bad AVP number %s\n", exports.name, (char*)*param);
			return E_CFG;
		}
	}
	else {
		re->type = AVP_NAME_RE;
		if (regcomp(&re->u.re, *param, REG_EXTENDED|REG_ICASE|REG_NEWLINE) ){
			pkg_free(re);
			LOG(L_ERR, "ERROR: %s : bad re %s\n", exports.name, (char*)*param);
			return E_BAD_RE;
		}
	}
	/* free string */
	pkg_free(*param);
	/* replace it with the compiled re */
	*param=re;
	return 0;
}

