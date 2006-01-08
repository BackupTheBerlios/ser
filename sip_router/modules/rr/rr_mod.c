/*
 * Route & Record-Route module
 *
 * $Id: rr_mod.c,v 1.40 2006/01/08 23:53:06 mma Exp $
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
#include "../../usr_avp.h"

#ifdef ENABLE_USER_CHECK
#include <string.h>
#include "../../str.h"
str i_user = STR_NULL;
#endif

int append_fromtag = 1;
int enable_double_rr = 1; /* Enable using of 2 RR by default */
int enable_full_lr = 0;   /* Disabled by default */
int add_username = 0;     /* Do not add username by default */
char *cookie_filter = 0;  /* filter cookies restored in loose_route, potential security problem */
str user_part_avp = STR_NULL;  /* AVP identification where user part of Route URI is stored after loose/strict routing */
avp_ident_t user_part_avp_ident;

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
	{"append_fromtag",   PARAM_INT,    &append_fromtag  },
	{"enable_double_rr", PARAM_INT,    &enable_double_rr},
	{"enable_full_lr",   PARAM_INT,    &enable_full_lr  },
#ifdef ENABLE_USER_CHECK
	{"ignore_user",      PARAM_STR,    &i_user     },
#endif
	{"add_username",     PARAM_INT,    &add_username    },
	{"cookie_filter",    PARAM_STRING, &cookie_filter   },
	{"user_part_avp",    PARAM_STR,    &user_part_avp   },	
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
	register_script_cb(rr_before_script_cb, REQ_TYPE_CB | PRE_SCRIPT_CB, 0);
	if (cookie_filter && strlen(cookie_filter)) {
		if (regcomp(cookie_filter_re, cookie_filter, REG_EXTENDED|REG_ICASE|REG_NEWLINE) ) {
			LOG(L_ERR, "ERROR: %s : bad cookie_filter regex '%s'\n", exports.name, cookie_filter);
			return E_BAD_RE;
		}
	}
	
	memset (&user_part_avp_ident, 0, sizeof(avp_ident_t));
	if (user_part_avp.s && user_part_avp.len) {
		if (parse_avp_ident(&user_part_avp, &user_part_avp_ident)!=0) {
			ERR("modparam \"user_part_avp\" : error while parsing\n");
			return E_CFG; 
		}
	}
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
			LOG(L_ERR, "ERROR: %s : bad AVP number '%s'\n", exports.name, (char*)*param);
			return E_CFG;
		}
	}
	else {
		re->type = AVP_NAME_RE;
		if (regcomp(&re->u.re, *param, REG_EXTENDED|REG_ICASE|REG_NEWLINE) ){
			pkg_free(re);
			LOG(L_ERR, "ERROR: %s : bad regex '%s'\n", exports.name, (char*)*param);
			return E_BAD_RE;
		}
	}
	/* free string */
	pkg_free(*param);
	/* replace it with the compiled re */
	*param=re;
	return 0;
}

