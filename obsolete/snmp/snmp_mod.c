/*
 * $Id: snmp_mod.c,v 1.2 2002/09/19 12:23:55 jku Rel $
 *
 * SNMP Module
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


/* 
 * - Needs net-snmp 5.0 or newer (http://www.net-snmp.org)
 * - Implemented as an agentx subagent. When the module is loaded, a new
 *   process is forked which contacts the master SNMP agent running on
 *   the system and registers the SIP MIB. If the master SNMP agent dies,
 *   we continue trying to contact it at 60 seconds intervals (chk
 *   SER_SNMP_PING in snmp_mod.h). If the agent comes back up, the SIP
 *   MIB is re-registered.
 * - 
 */

/*
 * Shortcomings:
 * - For now we need to run as root in order to be able to attach to 
 *   the master snmpd since agentx connections are expected on a unix socket
 *   which can only be read/written by root (/var/agentx/master). 
 *   To be able to attach as non-root the easiest way is to start snmpd 
 *   listening for agentx connections on a network port
 * - Current code assumes we're the only SIP entity on the system that's
 *   using SNMP (since we're the ones registering the MIB). To have 
 *   many SIP entities that can all be managed we'd need a centralized 
 *   authority in charge of managing the SIP MIB. Maybe convince the 
 *   net-snmp folks to add SIP to their agent?
 */

/* ser stuff */
#include "../../sr_module.h"
#include "../../dprint.h"
#include "snmp_handler.h"

/* our stuff */
extern int init_snmpVars();
extern int ser_init_snmp();
extern int ser_snmp_start();
extern int ser_snmp_stop();
extern int fill_sip_mibs();

/* XXX: Deprecated */
#if 0
int collect_ser_info();
char *listen_addr;
int listen_addr_len;
#endif

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>

static int mod_init(void);
static void mod_destroy(void);

struct module_exports exports = {
	"snmp",
	(char*[]){	/* script name of functions exported */
		"snmp_register_handler",
		"snmp_register_row",
		"snmp_register_table",
		"snmp_new_handler",
		"snmp_free_handler",
		"snmp_new_obj",
		"snmp_free_obj",
		"snmp_start",
		"snmp_stop"
	},
	(cmd_function[]){	/* the actual function */
		(cmd_function)snmp_register_handler,
		(cmd_function)snmp_register_row,
		(cmd_function)snmp_register_table,
		(cmd_function)snmp_new_handler,
		(cmd_function)snmp_free_handler,
		(cmd_function)snmp_new_obj,
		(cmd_function)snmp_free_obj,
		(cmd_function)ser_snmp_start,
		(cmd_function)ser_snmp_stop
	},
	(int[]){	/* number of params for ea function */
		2,
		2,
		2,
		1,
		1,
		1,
		1,
		0,
		0
	},
	(fixup_function[]){
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	},
	9,					/* how many functions */
	(char*[]){NULL},	/* module parameters */
	(modparam_t[]){0},	/* module params types */
	(void*[]){NULL},	/* actual parameters */
	0,					/* num of parameters */
	mod_init,			/* init function */
	NULL,				/* responses function, returns yes/no */
	mod_destroy,		/* module destroy function */
	NULL,				/* onbreak (script aborted) */
	NULL,				/* child init (init for each of child servers) */
};

/* Doesn't fork the agent. Must call ser_start_snmp() to complete
 * initialization */
static int mod_init(void)
{
	LOG(L_DBG, "snmp_mod: snmp being initialized...\n");

	/* Initialize any variables that we need to share across processes */
	LOG(L_DBG, "snmp_mod: Initializing internal variables\n");
	if(init_snmpVars() == -1) {
		LOG(L_ERR, "snmp_mod: Failed to initialize internal variables\n");
		return -1;
	}

	/* initialize snmp */
	LOG(L_DBG, "snmp_mod: Initializing snmp\n");
	if(ser_init_snmp() == -1) {
		LOG(L_ERR, "snmp_mod: Error initializing snmp library\n");
		return -1;
	}

	/* Initialize dynamic handler */
	LOG(L_DBG, "snmp_mod: Initializing dynamic handler\n");
	if(handler_init() == -1) {
		LOG(L_ERR, "snmp_mod: Failed to initialize dynamic handler\n");
		return -1;
	}

	/* Fill the parts we support internally */
	LOG(L_DBG, "snmp_mod: Filling SNMP tree\n");
	if(fill_sip_mibs() == -1) {
		LOG(L_ERR, "snmp_mod: Failed to fill the SIP MIB tree\n");
		return -1;
	}

	LOG(L_INFO, "SNMP module initialized\n");
	return 0;
}

static void mod_destroy(void)
{
	ser_snmp_stop();
}

/* collect info from ser for sipServerCfgTable */
/* XXX: Deprecated, should be done by sipServerCfgTable handler */
#if 0
int collect_ser_info()
{
	/* find listen address */
#include "../../config.h"
	extern struct socket_info sock_info[MAX_LISTEN];

	/* XXX: We can only use one for snmp stuff, so we choose the first one...
	 * chk sipServerCfgTable */
	listen_addr_len = sock_info[0].address_str.len;
	listen_addr = calloc(listen_addr_len+1, sizeof(char));
	if(!listen_addr) {
		LOG(L_ERR, "snmp_mod: Failed collecting vital ser info: %s\n",
				strerror(errno));
		return -1;
	}

	strncpy(listen_addr, sock_info[0].address_str.s, listen_addr_len+1);

	LOG(L_DBG, "snmp_mod: listen address is %s\n", listen_addr);

	return 0;
}
#endif
