/*
 * $Id: db.c,v 1.24 2008/01/16 14:13:54 janakj Exp $
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
 /*
  * History:
  * --------
  *  2004-06-06  bind_dbmod takes dbf as parameter (andrei)
  */

/** \ingroup DB_API
 * @{
 */

#include "db.h"

#include "db_drv.h"

#include "../dprint.h"
#include "../sr_module.h"
#include "../mem/mem.h"
#include "../str.h"
#include "../ut.h"
#include "../list.h"

struct _db_root db_root = DBLIST_INITIALIZER(db_root);

/** @} */
