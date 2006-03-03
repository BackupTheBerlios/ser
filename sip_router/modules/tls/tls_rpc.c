/*
 * $Id: tls_rpc.c,v 1.1 2006/03/03 11:26:53 janakj Exp $
 *
 * TLS module interface
 *
 * Copyright (C) 2001-2003 FhG FOKUS
 * Copyright (C) 2004,2005 Free Software Foundation, Inc.
 * Copyright (C) 2005 iptelorg GmbH
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

#include "../../rpc.h"
#include "tls_mod.h"
#include "tls_domain.h"
#include "tls_config.h"
#include "tls_util.h"
#include "tls_rpc.h"

static const char* tls_reload_doc[2] = {
	"Reload TLS configuration file",
	0
};

static void tls_reload(rpc_t* rpc, void* ctx)
{
	tls_cfg_t* cfg;
	
	if (!tls_cfg_file.s) {
		rpc->fault(ctx, 500, "No TLS configuration file configured");
		return;
	}

	     /* Try to delete old configurations first */
	collect_garbage();

	cfg = tls_load_config(&tls_cfg_file);
	if (!cfg) {
		rpc->fault(ctx, 500, "Error while loading TLS configuration file (consult server log)");
		return;
	}

	if (tls_fix_cfg(cfg, &srv_defaults, &cli_defaults) < 0) {
		rpc->fault(ctx, 500, "Error while fixing TLS configuration (consult server log)");
		goto error;
	}
	if (tls_check_sockets(cfg) < 0) {
		rpc->fault(ctx, 500, "No server listening socket found for one of TLS domains (consult server log)");
		goto error;
	}

	DBG("TLS configuration successfuly loaded");
	cfg->next = (*tls_cfg);
	*tls_cfg = cfg;
	return;

 error:
	tls_free_cfg(cfg);
	
}

rpc_export_t tls_rpc[] = {
	{"tls.reload", tls_reload, tls_reload_doc, 0},
	{0, 0, 0, 0}
};
