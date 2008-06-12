/* 
 * $Id: db_con.h,v 1.14 2008/06/12 16:11:46 alfredh Exp $ 
 *
 * Copyright (C) 2001-2003 FhG FOKUS
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

#ifndef _DB_CON_H
#define _DB_CON_H  1

/** \ingroup DB_API 
 * @{ 
 */

#include "db_gen.h"
#include "db_ctx.h"
#include "db_uri.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct db_con;
struct db_ctx;

typedef int (db_con_connect_t)(struct db_con* con);
typedef void (db_con_disconnect_t)(struct db_con* con);


typedef struct db_con {
	db_gen_t gen;            /* Generic part of the structure */
	db_con_connect_t* connect;
	db_con_disconnect_t* disconnect;

	struct db_ctx* ctx;
	db_uri_t* uri;
} db_con_t;

struct db_con* db_con(struct db_ctx* ctx, db_uri_t* uri);
void db_con_free(struct db_con* con);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* _DB_CON_H */


