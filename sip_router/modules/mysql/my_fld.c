/* 
 * $Id: my_fld.c,v 1.4 2007/11/15 17:27:06 janakj Exp $
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

#include <string.h>
#include "../../mem/mem.h"
#include "../../dprint.h"
#include "../../db/db_gen.h"
#include "my_fld.h"


static void my_fld_free(db_fld_t* fld, struct my_fld* payload)
{
	db_drv_free(&payload->gen);
	if (payload->buf.s) pkg_free(payload->buf.s);
	if (payload->name) pkg_free(payload->name);
	pkg_free(payload);
}


int my_fld(db_fld_t* fld, char* table)
{
	struct my_fld* res;

	res = (struct my_fld*)pkg_malloc(sizeof(struct my_fld));
	if (res == NULL) {
		ERR("No memory left\n");
		return -1;
	}
	memset(res, '\0', sizeof(struct my_fld));
	if (db_drv_init(&res->gen, my_fld_free) < 0) goto error;

	DB_SET_PAYLOAD(fld, res);
	return 0;

 error:
	if (res) pkg_free(res);
	return -1;
}
