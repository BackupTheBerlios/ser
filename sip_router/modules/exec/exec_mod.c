/*
 * execution module
 *
 * $Id: exec_mod.c,v 1.6 2003/02/28 14:12:26 jiri Exp $
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

#include "../../parser/msg_parser.h"
#include "../../str.h"
#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../parser/parse_uri.h"

#include "exec.h"
#include "kill.h"
#include "exec_hf.h"

unsigned int time_to_kill=0;

static int mod_init( void );

inline static int w_exec_dset(struct sip_msg* msg, char* cmd, char* foo);
inline static int w_exec_msg(struct sip_msg* msg, char* cmd, char* foo);

inline static void exec_shutdown();

#ifdef STATIC_EXEC
struct module_exports exec_exports = {
#else
struct module_exports exports= {
#endif
	"exec",

	/* exported functions */
	( char*[] ) { "exec_dset", "exec_msg" },
	( cmd_function[] ) { w_exec_dset, w_exec_msg },
	( int[] ) { 1, 1 /* params == cmd name */ }, 
	( fixup_function[]) { 0, 0 },
	2, /* number of exported functions */

	/* exported variables */
	(char *[]) { /* variable names */
		"time_to_kill", "setvars"
	},

	(modparam_t[]) { /* variable types */
		INT_PARAM, /* time_to_kill */
		INT_PARAM, /* set vars */
	},

	(void *[]) { /* variable pointers */
		&time_to_kill,
		&setvars,
	},

	2,			/* number of variables */

	mod_init, 	/* initialization module */
	0,			/* response function */
	exec_shutdown,	/* destroy function */
	0,			/* oncancel function */
	0			/* per-child init function */
};

void exec_shutdown()
{
	if (time_to_kill) destroy_kill();
}


static int mod_init( void )
{
	fprintf( stderr, "exec - initializing\n");
	if (time_to_kill) initialize_kill();
	return 0;
}

inline static int w_exec_dset(struct sip_msg* msg, char* cmd, char* foo)
{
	str *uri;
	environment_t *backup;
	int ret;

	backup=0;
	if (setvars) {
		backup=set_env(msg);
		if (!backup) {
			LOG(L_ERR, "ERROR: w_exec_msg: no env created\n");
			return -1;
		}
	}

	if (msg->new_uri.s && msg->new_uri.len)
		uri=&msg->new_uri;
	else
		uri=&msg->first_line.u.request.uri;

	ret=exec_str(msg, cmd, uri->s, uri->len);
	if (setvars) {
		unset_env(backup);
	}
	return ret;
}


inline static int w_exec_msg(struct sip_msg* msg, char* cmd, char* foo)
{
	environment_t *backup;
	int ret;

	backup=0;
	if (setvars) {
		backup=set_env(msg);
		if (!backup) {
			LOG(L_ERR, "ERROR: w_exec_msg: no env created\n");
			return -1;
		}
	}
	ret=exec_msg(msg,cmd);
	if (setvars) {
		unset_env(backup);
	}
	return ret;
}
