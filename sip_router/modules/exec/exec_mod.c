/*
 * execution module
 *
 * $Id: exec_mod.c,v 1.2 2002/08/19 19:47:29 jku Exp $
 */

#include <stdio.h>

#include "../../parser/msg_parser.h"
#include "../../str.h"
#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../parser/parse_uri.h"

#include "exec.h"
#include "kill.h"

unsigned int time_to_kill=0;

static int mod_init( void );

inline static int w_exec_uri(struct sip_msg* msg, char* cmd, char* foo);
inline static int w_exec_user(struct sip_msg* msg, char* cmd, char* foo);
inline static int w_exec_msg(struct sip_msg* msg, char* cmd, char* foo);

inline static void exec_shutdown();

#ifdef STATIC_EXEC
struct module_exports exec_exports = {
#else
struct module_exports exports= {
#endif
	"exec",

	/* exported functions */
	( char*[] ) { "exec_uri", "exec_user", "exec_msg" },
	( cmd_function[] ) { w_exec_uri, w_exec_user, w_exec_msg },
	( int[] ) { 1, 1, 1 /* params == cmd name */ }, 
	( fixup_function[]) { 0, 0 , 0 },
	3, /* number of exported functions */

	/* exported variables */
	(char *[]) { /* variable names */
		"time_to_kill"
	},

	(modparam_t[]) { /* variable types */
		INT_PARAM, /* time_to_kill */
	},

	(void *[]) { /* variable pointers */
		&time_to_kill
	},

	1,			/* number of variables */

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

inline static int w_exec_uri(struct sip_msg* msg, char* cmd, char* foo)
{
	str *uri;

	if (msg->new_uri.s && msg->new_uri.len)
		uri=&msg->new_uri;
	else
		uri=&msg->first_line.u.request.uri;

	return exec_str(msg, cmd, uri->s);
}
inline static int w_exec_user(struct sip_msg* msg, char* cmd, char* foo)
{
	str *uri;
	struct sip_uri parsed_uri;

	if (msg->new_uri.s && msg->new_uri.len)
		uri=&msg->new_uri;
	else
		uri=&msg->first_line.u.request.uri;
	if (parse_uri(uri->s, uri->len, &parsed_uri )<0) {
		LOG(L_ERR, "ERROR: invalid URI passed to w_exec_user\n");
		return -1;
	}
	if (!parsed_uri.user.s || !parsed_uri.user.len) {
		LOG(L_WARN, "WARNING: w_exec_user: "
			"empty username in '%.*s'\n", uri->len, uri->s );
		return exec_str(msg, cmd, "" );
	}
	return exec_str(msg, cmd, parsed_uri.user.s);
}

inline static int w_exec_msg(struct sip_msg* msg, char* cmd, char* foo)
{
	return exec_msg(msg,cmd);
}
