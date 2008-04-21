/**
 * $Id: ds_rpc.h,v 1.1 2008/04/21 13:15:16 hscholz Exp $
 *
 * dispatcher module -- stateless load balancing
 *
 * Copyright (C) 2004-2006 FhG Fokus
 * Copyright (C) 2005-2008 Hendrik Scholz <hendrik.scholz@freenet-ag.de>
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

#ifndef _DS_RPC_H_
#define _DS_RPC_H_

#include "../../sr_module.h"
#include "dispatcher.h"

/* rpc function titles */
static const char *rpc_dump_doc[2] = {
	"Dump dispatcher set configuration",
	0
};
static const char *rpc_reload_doc[2] = {
	"Reload dispatcher list from file",
	0
};

/* prototypes */
void rpc_dump(rpc_t *rpc, void *c);
void rpc_reload(rpc_t *rpc, void *c);

#endif /* _DS_RPC_H_ */
