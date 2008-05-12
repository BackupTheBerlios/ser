/* 
 * $Id: ld_con.h,v 1.1 2008/05/12 12:14:38 janakj Exp $ 
 *
 * LDAP Database Driver for SER
 *
 * Copyright (C) 2008 iptelorg GmbH
 *
 * This file is part of SER, a free SIP server.
 *
 * SER is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version
 *
 * For a license to use the ser software under conditions other than those
 * described here, or to purchase support for this software, please contact
 * iptel.org by e-mail at the following addresses: info@iptel.org
 *
 * SER is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LD_CON_H
#define _LD_CON_H

/** \addtogroup ldap
 * @{ 
 */

/** \file 
 * Implementation of LDAP per-connection related data structures and functions.
 */

#include "../../db/db_pool.h"
#include "../../db/db_con.h"
#include "../../db/db_uri.h"

#include <time.h>
#include <ldap.h>

/** 
 * Per-connection flags for LDAP connections.
 */
enum ld_con_flags {
	LD_CONNECTED      = (1 << 0), /**< The connection has been connected successfully */
};


/** A structure representing a connection to a LDAP server.
 * This structure represents connections to LDAP servers. It contains
 * LDAP specific per-connection data, 
 */
struct ld_con {
	db_pool_entry_t gen;  /**< Generic part of the structure */
	LDAP* con;            /**< LDAP connection handle */
	unsigned int flags;   /**< Flags */
};


/** Create a new ld_con structure.
 * This function creates a new ld_con structure and attachs the structure to
 * the generic db_con structure in the parameter.
 * @param con A generic db_con structure to be extended with LDAP payload
 * @retval 0 on success
 * @retval A negative number on error
 */
int ld_con(db_con_t* con);


/** Establish a new connection to server.  
 * This function is called when a SER module calls db_connect to establish a
 * new connection to the database server. After the connection is established
 * the function sends an SQL query to the server to determine the format of
 * timestamp fields and also obtains the list of supported field types.
 * @param con A structure representing database connection.
 * @retval 0 on success.
 * @retval A negative number on error.
 */
int ld_con_connect(db_con_t* con);


/** Disconnected from LDAP server.
 * Disconnects a previously connected connection to LDAP server.
 * @param con A structure representing the connection to be disconnected.
 */
void ld_con_disconnect(db_con_t* con);

/** @} */

#endif /* _LD_CON_H */
