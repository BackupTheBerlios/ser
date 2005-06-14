/*
 * $Id: avp_db.h,v 1.1 2005/06/14 14:32:02 kozlik Exp $
 *
 * Copyright (C) 2004 FhG Fokus
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

#include "../../db/db.h"

#define AVP_LIST_RELOAD "avp_list_reload"
 
extern db_con_t* db_handle;
extern db_func_t dbf;

extern char* db_list_table;
extern char* attr_name_column;
extern char* attr_type_column;
extern char* attr_dval_column;


int avp_db_init();
void avp_db_close();
