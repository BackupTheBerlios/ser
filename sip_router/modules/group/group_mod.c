/* 
 * $Id: group_mod.c,v 1.17 2005/11/30 17:06:47 janakj Exp $ 
 *
 * Group membership - module interface
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
 *  2003-02-25 - created by janakj
 *  2003-03-11 - New module interface (janakj)
 *  2003-03-16 - flags export parameter added (janakj)
 *  2003-03-19  all mallocs/frees replaced w/ pkg_malloc/pkg_free
 *  2003-04-05  default_uri #define used (jiri)
 *  2004-06-07  updated to the new DB api: calls to group_db_* (andrei)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../ut.h"
#include "../../error.h"
#include "../../mem/mem.h"
#include "group_mod.h"
#include "group.h"

MODULE_VERSION

#define TABLE_VERSION 3

/*
 * Module destroy function prototype
 */
static void destroy(void);


/*
 * Module child-init function prototype
 */
static int child_init(int rank);


/*
 * Module initialization function prototype
 */
static int mod_init(void);


/* Header field fixup */
static int hf_fixup(void** param, int param_no);


#define TABLE "grp"

#define UID_COL "uid"
#define GROUP_COL "grp"


/*
 * Module parameter variables
 */
static str db_url        = STR_STATIC_INIT(DEFAULT_RODB_URL);
str table         = STR_STATIC_INIT(TABLE);         /* Table name where group definitions are stored */
str uid_column   = STR_STATIC_INIT(UID_COL);
str group_column  = STR_STATIC_INIT(GROUP_COL);


/*
 * Exported functions
 */
static cmd_export_t cmds[] = {
	{"is_user_in", is_user_in, 1, hf_fixup, REQUEST_ROUTE},
	{0, 0, 0, 0, 0}
};


/*
 * Exported parameters
 */
static param_export_t params[] = {
	{"db_url",        STR_PARAM, &db_url.s       },
	{"table",         STR_PARAM, &table.s        },
	{"uid_column",    STR_PARAM, &uid_column.s  },
	{"group_column",  STR_PARAM, &group_column.s },
	{0, 0, 0}
};


/*
 * Module interface
 */
struct module_exports exports = {
	"group", 
	cmds,       /* Exported functions */
	params,     /* Exported parameters */
	mod_init,   /* module initialization function */
	0,          /* response function */
	destroy,    /* destroy function */
	0,          /* oncancel function */
	child_init  /* child initialization function */
};


static int child_init(int rank)
{
	return group_db_init(db_url.s);
}


static int mod_init(void)
{
	int ver;

	DBG("group module - initializing\n");

	     /* Calculate lengths */
	db_url.len = strlen(db_url.s);
	table.len = strlen(table.s);
	uid_column.len = strlen(uid_column.s);
	group_column.len = strlen(group_column.s);

	     /* Find a database module */
	if (group_db_bind(db_url.s)) {
		return -1;
	}
	ver = group_db_ver(db_url.s, &table);
	if (ver < 0) {
		LOG(L_ERR, "group:mod_init(): Error while querying table version\n");
		return -1;
	} else if (ver < TABLE_VERSION) {
		LOG(L_ERR, "group:mod_init(): Invalid table version "
				"(use ser_mysql.sh reinstall)\n");
		return -1;
	}
	
	return 0;
}


static void destroy(void)
{
	group_db_close();
}


/*
 * Convert HF description string to hdr_field pointer
 *
 * Supported strings: 
 * "$t.uid", "$f.uid"
 */
static int hf_fixup(void** param, int param_no)
{
	void* ptr;

	if (param_no == 1) {
		ptr = *param;
		
		if (!strcasecmp((char*)*param, "$t.uid")) {
			*param = (void*)1;
		} else if (!strcasecmp((char*)*param, "$f.uid")) {
			*param = (void*)2;
		} else {
			LOG(L_ERR, "group:hf_fixup: Unsupported Header Field identifier\n");
			return E_UNSPEC;
		}

		pkg_free(ptr);
	}
	return 0;
}

