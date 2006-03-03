/*
 * $Id: tls_mod.c,v 1.5 2006/03/03 12:26:07 janakj Exp $
 *
 * TLS module interface
 *
 * Copyright (C) 2001-2003 FhG FOKUS
 * Copyright (C) 2004,2005 Free Software Foundation, Inc.
 * Copyright (C) 2005,2006 iptelorg GmbH
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
 * 2003-04-05: default_uri #define used (jiri)
 * 2003-04-06: db connection closed in mod_init (janakj)
 * 2004-06-06  updated to the new DB api, cleanup: static dbf & handler,
 *              calls to domain_db_{bind,init,close,ver} (andrei)
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../../locking.h"
#include "../../sr_module.h"
#include "../../ip_addr.h"
#include "../../trim.h"
#include "../../transport.h"
#include "../../globals.h"
#include "tls_init.h"
#include "tls_server.h"
#include "tls_domain.h"
#include "tls_select.h"
#include "tls_config.h"
#include "tls_rpc.h"
#include "tls_mod.h"


/*
 * FIXME:
 * - How do we ask for secret key password ? Mod_init is called after
 *   daemonize and thus has no console access
 * - forward_tls and t_relay_to_tls should be here
 * add tls_log
 * - Currently it is not possible to reset certificate in a domain,
 *   for example if you specify client certificate in the default client
 *   domain then there is no way to define another client domain which would
 *   have no client certificate configured
 */


/*
 * Module management function prototypes
 */
static int mod_init(void);
static void destroy(void);

MODULE_VERSION


/*
 * Default settings when modparams are used 
 */
static tls_domain_t mod_params = {
	TLS_DOMAIN_DEF | TLS_DOMAIN_SRV,   /* Domain Type */
	{},               /* IP address */
	0,                /* Port number */
	0,                /* SSL ctx */
	TLS_CERT_FILE,    /* Certificate file */
	TLS_PKEY_FILE,    /* Private key file */
	0,                /* Verify certificate */
	9,                /* Verify depth */
	TLS_CA_FILE,      /* CA file */
	0,                /* Require certificate */
	0,                /* Cipher list */
	TLS_USE_TLSv1,    /* TLS method */
	0                 /* next */
};


/*
 * Default settings for server domains when using external config file
 */
tls_domain_t srv_defaults = {
	TLS_DOMAIN_DEF | TLS_DOMAIN_SRV,   /* Domain Type */
	{},               /* IP address */
	0,                /* Port number */
	0,                /* SSL ctx */
	TLS_CERT_FILE,    /* Certificate file */
	TLS_PKEY_FILE,    /* Private key file */
	0,                /* Verify certificate */
	9,                /* Verify depth */
	TLS_CA_FILE,      /* CA file */
	0,                /* Require certificate */
	0,                /* Cipher list */
	TLS_USE_TLSv1,    /* TLS method */
	0                 /* next */
};


/*
 * Default settings for client domains when using external config file
 */
tls_domain_t cli_defaults = {
	TLS_DOMAIN_DEF | TLS_DOMAIN_CLI,   /* Domain Type */
	{},               /* IP address */
	0,                /* Port number */
	0,                /* SSL ctx */
	0,                /* Certificate file */
	0,                /* Private key file */
	0,                /* Verify certificate */
	9,                /* Verify depth */
	TLS_CA_FILE,      /* CA file */
	0,                /* Require certificate */
	0,                /* Cipher list */
	TLS_USE_TLSv1,    /* TLS method */
	0                 /* next */
};


/*
 * Defaults for client and server domains when using modparams
 */
static str tls_method = STR_STATIC_INIT("TLSv1");


int tls_handshake_timeout = 120;
int tls_send_timeout = 120;
int tls_conn_timeout = 600;
int tls_log = 3;
int tls_session_cache = 0;
str tls_session_id = STR_STATIC_INIT("ser-tls-0.9.0");
str tls_cfg_file = STR_NULL;


/* Current TLS configuration */
tls_cfg_t** tls_cfg = NULL;

/* List lock, used by garbage collector */
gen_lock_t* tls_cfg_lock = NULL;


/*
 * Exported functions
 */
static cmd_export_t cmds[] = {
	{0, 0, 0, 0, 0}
};


/*
 * Exported parameters
 */
static param_export_t params[] = {
	{"tls_method",          PARAM_STR,    &tls_method             },
	{"verify_certificate",  PARAM_INT,    &mod_params.verify_cert },
	{"verify_depth",        PARAM_INT,    &mod_params.verify_depth},
	{"require_certificate", PARAM_INT,    &mod_params.require_cert},
	{"private_key",         PARAM_STRING, &mod_params.pkey_file   },
	{"ca_list",             PARAM_STRING, &mod_params.ca_file     },
	{"certificate",         PARAM_STRING, &mod_params.cert_file   },
	{"cipher_list",         PARAM_STRING, &mod_params.cipher_list },
	{"handshake_timeout",   PARAM_INT,    &tls_handshake_timeout  },
	{"send_timeout",        PARAM_INT,    &tls_send_timeout       },
	{"connection_timeout",  PARAM_INT,    &tls_conn_timeout       },
	{"tls_log",             PARAM_INT,    &tls_log                },
	{"session_cache",       PARAM_INT,    &tls_session_cache      },
	{"session_id",          PARAM_STR,    &tls_session_id         },
	{"config",              PARAM_STR,    &tls_cfg_file           },
	{0, 0, 0}
};


/*
 * Module interface
 */
struct module_exports exports = {
	"tls",
	cmds,       /* Exported functions */
	tls_rpc,    /* RPC methods */
	params,     /* Exported parameters */
	mod_init,   /* module initialization function */
	0,          /* response function*/
	destroy,    /* destroy function */
	0,          /* cancel function */
	0           /* per-child init function */
};



transport_t tls_transport = {
	PROTO_TLS,
	STR_STATIC_INIT("TLS"),
	TRANSPORT_SECURE | TRANSPORT_STREAM,
	{ 
		.tcp = {
			tls_tcpconn_init,
			tls_tcpconn_clean,
			tls_close,
			tls_blocking_write,
			tls_read,
			tls_fix_read_conn,
		}
	},
	0
};


/*
 * Create TLS configuration from modparams
 */
static tls_cfg_t* tls_use_modparams(void)
{
	tls_cfg_t* ret;
	
	ret = tls_new_cfg();
	if (!ret) return;

	
}


static int mod_init(void)
{
	int method;

	     /* Convert tls_method parameter to integer */
	method = tls_parse_method(&tls_method);
	if (method < 0) {
		ERR("Invalid tls_method parameter value\n");
		return -1;
	}
	mod_params.method = method;

	tls_cfg = (tls_cfg_t**)shm_malloc(sizeof(tls_cfg_t*));
	if (!tls_cfg) {
		ERR("Not enough shared memory left\n");
		return -1;
	}
	*tls_cfg = NULL;

	tls = &tls_transport;
	register_select_table(tls_sel);

	if (init_tls() < 0) return -1;
	
	tls_cfg_lock = lock_alloc();
	if (tls_cfg_lock == 0) {
		ERR("Unable to create TLS configuration lock\n");
		return -1;
	}
	if (lock_init(tls_cfg_lock) == 0) {
		lock_dealloc(tls_cfg_lock);
		ERR("Unable to initialize TLS configuration lock\n");
		return -1;
	}

	if (tls_cfg_file.s) {
		*tls_cfg = tls_load_config(&tls_cfg_file);
		if (!(*tls_cfg)) return -1;
		if (tls_fix_cfg(*tls_cfg, &srv_defaults, &cli_defaults) < 0) return -1;
	} else {
		*tls_cfg = tls_new_cfg();
		if (!(*tls_cfg)) return -1;
		if (tls_fix_cfg(*tls_cfg, &mod_params, &mod_params) < 0) return -1;
	}

	if (tls_check_sockets(*tls_cfg) < 0) return -1;

	return 0;
}


static void destroy(void)
{
	tls_cfg_t* ptr;

	lock_destroy(tls_cfg_lock);
	lock_dealloc(tls_cfg_lock);

	while(*tls_cfg) {
		ptr = *tls_cfg;
		*tls_cfg = (*tls_cfg)->next;
		tls_free_cfg(ptr);
	}

	shm_free(tls_cfg);
	destroy_tls();
}
