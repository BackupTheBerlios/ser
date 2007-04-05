/*
 * $Id: db.h,v 1.23 2007/04/05 11:37:52 janakj Exp $
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

#ifndef _DB_H
#define _DB_H  1

/**
 * \defgroup DB_API Database Abstraction Layer
 *
 * @{
 */


#include "db_gen.h"
#include "db_ctx.h"
#include "db_uri.h"
#include "db_cmd.h"
#include "db_res.h"
#include "db_rec.h"
#include "db_fld.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*
 * Various database flags shared by modules 
 */
#define DB_LOAD_SER   (1 << 0)  /* The row should be loaded by SER */
#define DB_DISABLED   (1 << 1)  /* The row is disabled */
#define DB_CANON      (1 << 2)  /* Canonical entry (domain or uri) */
#define DB_IS_TO      (1 << 3)  /* The URI can be used in To */
#define DB_IS_FROM    (1 << 4)  /* The URI can be used in From */
#define DB_FOR_SERWEB (1 << 5)  /* Credentials instance can be used by serweb */
#define DB_PENDING    (1 << 6)
#define DB_DELETED    (1 << 7)
#define DB_CALLER_DELETED (1 << 8) /* Accounting table */
#define DB_CALLEE_DELETED (1 << 9) /* Accounting table */
#define DB_MULTIVALUE     (1 << 10) /* Attr_types table */
#define DB_FILL_ON_REG    (1 << 11) /* Attr_types table */
#define DB_REQUIRED       (1 << 12) /* Attr_types table */
#define DB_DIR            (1 << 13) /* Domain_settings table */


struct db_gen;


DBLIST_HEAD(_db_root);

/** \brief The root of all DB API structures
 *
 *  This is the root linked list of all database
 *  structures allocated in SER
 */
extern struct _db_root db_root;

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* _DB_H */
