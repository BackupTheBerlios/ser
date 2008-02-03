/*
 * $Id: mysql_mod.c,v 1.8 2008/02/03 19:12:05 janakj Exp $
 *
 * MySQL module interface
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
 *  2003-03-11  updated to the new module exports interface (andrei)
 *  2003-03-16  flags export parameter added (janakj)
 */

/** @addtogroup mysql
 *  @{
 */
 
#include "mysql_mod.h"

#include "my_uri.h"
#include "my_con.h"
#include "my_cmd.h"
#include "my_fld.h"
#include "my_res.h"

#include "../../sr_module.h"
#include "../../db/db.h"

int my_ping_interval = 5 * 60; /* Default is 5 minutes */
unsigned int my_connect_to = 2; /* 2 s by default */
unsigned int my_send_to = 0; /*  enabled only for mysql >= 5.25  */
unsigned int my_recv_to = 0; /* enabled only for mysql >= 5.25 */
unsigned int my_retries = 1;    /* Number of retries when command fails */

unsigned long my_client_ver = 0;

#define DEFAULT_MY_SEND_TO  2   /* in seconds */
#define DEFAULT_MY_RECV_TO  4   /* in seconds */

static int mysql_mod_init(void);

MODULE_VERSION


/*
 * MySQL database module interface
 */
static cmd_export_t cmds[] = {
	{"db_ctx",    (cmd_function)NULL,         0, 0, 0},
	{"db_con",    (cmd_function)my_con,       0, 0, 0},
	{"db_uri",    (cmd_function)my_uri,       0, 0, 0},
	{"db_cmd",    (cmd_function)my_cmd,       0, 0, 0},
	{"db_put",    (cmd_function)my_cmd_exec,  0, 0, 0},
	{"db_del",    (cmd_function)my_cmd_exec,  0, 0, 0},
	{"db_get",    (cmd_function)my_cmd_exec,  0, 0, 0},
	{"db_upd",    (cmd_function)my_cmd_exec,  0, 0, 0},
	{"db_sql",    (cmd_function)my_cmd_exec,  0, 0, 0},
	{"db_res",    (cmd_function)my_res,       0, 0, 0},
	{"db_fld",    (cmd_function)my_fld,       0, 0, 0},
	{"db_first",  (cmd_function)my_cmd_first, 0, 0, 0},
	{"db_next",   (cmd_function)my_cmd_next,  0, 0, 0},
	{"db_setopt", (cmd_function)my_setopt,    0, 0, 0},
	{"db_getopt", (cmd_function)my_getopt,    0, 0, 0},
	{0, 0, 0, 0, 0}
};


/*
 * Exported parameters
 */
static param_export_t params[] = {
	{"ping_interval",   PARAM_INT, &my_ping_interval},
	{"connect_timeout", PARAM_INT, &my_connect_to},
	{"send_timeout",    PARAM_INT, &my_send_to},
	{"receive_timeout", PARAM_INT, &my_recv_to},
	{"retries",         PARAM_INT, &my_retries},
	{0, 0, 0}
};


struct module_exports exports = {
	"mysql",
	cmds,
	0,               /* RPC method */
	params,          /*  module parameters */
	mysql_mod_init,  /* module initialization function */
	0,               /* response function*/
	0,               /* destroy function */
	0,               /* oncancel function */
	0                /* per-child init function */
};


static int mysql_mod_init(void)
{
#if MYSQL_VERSION_ID >= 40101
	my_client_ver = mysql_get_client_version();
	if ((my_client_ver >= 50025) || 
		((my_client_ver >= 40122) && 
		 (my_client_ver < 50000))) {
		if (my_send_to == 0) {
			my_send_to= DEFAULT_MY_SEND_TO;
		}
		if (my_recv_to == 0) {
			my_recv_to= DEFAULT_MY_RECV_TO;
		}
	} else if (my_recv_to || my_send_to) {
		LOG(L_WARN, "WARNING: mysql send or received timeout set, but "
			" not supported by the installed mysql client library"
			" (needed at least 4.1.22 or 5.0.25, but installed %ld)\n",
			my_client_ver);
	}
#else
	if (my_recv_to || my_send_to) {
		LOG(L_WARN, "WARNING: mysql send or received timeout set, but "
			" not supported by the mysql client library used to compile"
			" the mysql module (needed at least 4.1.1 but "
			" compiled against %ld)\n", MYSQL_VERSION_ID);
	}
#endif
	return 0;
}

/** @} */
