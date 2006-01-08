/*
 * $Id: enum_mod.c,v 1.13 2006/01/08 22:43:16 tma0 Exp $
 *
 * Enum module
 *
 * Copyright (C) 2002-2003 Juha Heinanen
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
 * 2003-03-11: New module interface (janakj)
 * 2003-03-16: flags export parameter added (janakj)
 * 2003-12-15: added suffix parameter to enum_query (jh)
 */


#include "enum_mod.h"
#include <stdio.h>
#include <stdlib.h>
#include "../../sr_module.h"
#include "../../error.h"
#include "enum.h"

MODULE_VERSION

/*
 * Module initialization function prototype
 */
static int mod_init(void);


/*
 * Module parameter variables
 */
char* domain_suffix = "e164.arpa.";
char* tel_uri_params = "";


/*
 * Internal module variables
 */
str suffix;
str param;
str service;


/*
 * Exported functions
 */
static cmd_export_t cmds[] = {
	{"enum_query",        enum_query_0,      0, 0,            REQUEST_ROUTE},
	{"enum_query",        enum_query_1,      1, fixup_str_1,  REQUEST_ROUTE},
	{"enum_query",        enum_query_2,      2, fixup_str_12, REQUEST_ROUTE},
	{"is_from_user_e164", is_from_user_e164, 0, 0,            REQUEST_ROUTE},
	{0, 0, 0, 0, 0}
};


/*
 * Exported parameters
 */
static param_export_t params[] = {
        {"domain_suffix", PARAM_STRING, &domain_suffix},
        {"tel_uri_params", PARAM_STRING, &tel_uri_params},
	{0, 0, 0}
};


/*
 * Module parameter variables
 */
struct module_exports exports = {
	"enum",
	cmds,     /* Exported functions */
	0,        /* RPC method */
	params,   /* Exported parameters */
	mod_init, /* module initialization function */
	0,        /* response function*/
	0,        /* destroy function */
	0,        /* oncancel function */
	0         /* per-child init function */
};


static int mod_init(void)
{
	DBG("enum module - initializing\n");

	suffix.s = domain_suffix;
	suffix.len = strlen(suffix.s);

	param.s = tel_uri_params;
	param.len = strlen(param.s);

	service.len = 0;

	return 0;
}

