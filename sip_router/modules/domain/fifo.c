/*
 * $Id: fifo.c,v 1.12 2005/11/21 16:43:14 janakj Exp $
 *
 * Domain fifo functions
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
 */
/*
 * History:
 * --------
 *  2004-06-07  moved reload_domain_table() into domain.c (andrei)
 */


#include "domain_mod.h"
#include "domain.h"
#include "hash.h"
#include "fifo.h"
#include "../../fifo_server.h"
#include "../../dprint.h"
#include "../../db/db.h"

/* FIXME: Check for value of db_mode and return immediately if set to 0 */


/*
 * Fifo function to reload domain table
 */
static int domain_reload(FILE* pipe, char* response_file)
{
	if (reload_domain_list() < 0) {
		fifo_reply (response_file, "400 Domain table reload failed\n");
		return -1;
	} else {
		fifo_reply (response_file, "200 OK\n");
		return 1;
	}
}


/*
 * Fifo function to print domains from current hash table
 */
static int domain_dump(FILE* pipe, char* response_file)
{
	domain_t* list;
	FILE *reply_file;

	if (db_mode == 0) {
		fifo_reply(response_file, "400 This command only works with domain memory cache enabled\n");
		return -1;
	}
	
	reply_file=open_reply_pipe(response_file);
	if (reply_file==0) {
		LOG(L_ERR, "domain:domain_dump: Opening of response file failed\n");
		return -1;
	}
	
	if (*active_hash == hash_1) list = *domains_1;
	else list = *domains_2;

	fputs( "200 OK\n", reply_file);
	dump_domain_list(reply_file, list);
	fclose(reply_file);
	return 1;
}


/*
 * Register domain fifo functions
 */
int init_domain_fifo(void)
{
	if (register_fifo_cmd(domain_reload, DOMAIN_RELOAD, 0) < 0) {
		LOG(L_CRIT, "Cannot register domain_reload\n");
		return -1;
	}

	if (register_fifo_cmd(domain_dump, DOMAIN_DUMP, 0) < 0) {
		LOG(L_CRIT, "Cannot register domain_dump\n");
		return -1;
	}

	return 1;
}
