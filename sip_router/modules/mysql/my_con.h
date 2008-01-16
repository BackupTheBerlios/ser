/* 
 * $Id: my_con.h,v 1.7 2008/01/16 14:17:28 janakj Exp $
 *
 * Copyright (C) 2001-2003 FhG Fokus
 * Copyright (C) 2006-2007 iptelorg GmbH
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

#ifndef _MY_CON_H
#define _MY_CON_H  1

#include "../../db/db_pool.h"
#include "../../db/db_con.h"
#include "../../db/db_uri.h"

#include <time.h>
#include <mysql/mysql.h>

enum my_flags {
	MY_CONNECTED = 1
};

typedef struct my_con {
	/* Generic part of the structure */
	db_pool_entry_t gen;

	MYSQL* con;
	unsigned int flags;
	
	/* We keep the number of connection resets in this variable,
	 * this variable is incremented each time the module performs
	 * a re-connect on the connection. This is used by my_cmd
	 * related functions to check if a pre-compiled command needs
	 * to be uploaded to the server before executing it.
	 */
	unsigned int resets;
} my_con_t;


/*
 * Create a new connection structure,
 * open the MySQL connection and set reference count to 1
 */
int my_con(db_con_t* con);

int my_con_connect(db_con_t* con);
void my_con_disconnect(db_con_t* con);

#endif /* _MY_CON_H */
