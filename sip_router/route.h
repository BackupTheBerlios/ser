/*
 * $Id: route.h,v 1.16 2007/06/14 23:12:26 andrei Exp $
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


#ifndef route_h
#define route_h

#include <sys/types.h>
#include <regex.h>
#include <netdb.h>

#include "config.h"
#include "error.h"
#include "route_struct.h"
#include "action.h"
#include "parser/msg_parser.h"
#include "str_hash.h"

/*#include "cfg_parser.h" */


struct route_list{
	struct action** rlist;
	int idx; /* first empty entry */ 
	int entries; /* total number of entries */
	struct str_hash_table names; /* name to route index mappings */
};


/* main "script table" */
extern struct route_list main_rt;
/* main reply route table */
extern struct route_list onreply_rt;
extern struct route_list failure_rt;
extern struct route_list branch_rt;
extern struct route_list onsend_rt;


int init_routes();
void destroy_routes();
int route_get(struct route_list* rt, char* name);
int route_lookup(struct route_list* rt, char* name);

void push(struct action* a, struct action** head);
int add_actions(struct action* a, struct action** head);
void print_rls();
int fix_rls();

int eval_expr(struct run_act_ctx* h, struct expr* e, struct sip_msg* msg);






#endif
