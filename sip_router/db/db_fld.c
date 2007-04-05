/* 
 * $Id: db_fld.c,v 1.5 2007/04/05 11:38:33 janakj Exp $ 
 *
 * Copyright (C) 2001-2005 FhG FOKUS
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

/** \ingroup DB_API 
 * @{ 
 */

#include <string.h>
#include "../mem/mem.h"
#include "../dprint.h"
#include "db_fld.h"


int db_fld_init(db_fld_t* fld)
{
	int i;

	for(i = 0; !DB_FLD_LAST(fld[i]); i++) {
		if (db_gen_init(&fld[i].gen) < 0) return -1;
	}
	return 0;
}


void db_fld_close(db_fld_t* fld)
{
	int i;

	for(i = 0; !DB_FLD_LAST(fld[i]); i++) {
		db_gen_free(&fld[i].gen);
	}
}


db_fld_t* db_fld(size_t n)
{
	int i;
	db_fld_t* newp;

	newp = (db_fld_t*)pkg_malloc(sizeof(db_fld_t) * n);
	if (newp == NULL) {
		ERR("db_fld: No memory left\n");
		return NULL;
	}
	memset(newp, '\0', sizeof(db_fld_t) * n);

	for(i = 0; i < n; i++) {
		if (db_gen_init(&newp[i].gen) < 0) goto error;
	}
	return newp;

 error:
	if (newp) {
		db_gen_free(&newp->gen);
		pkg_free(newp);
	}
	return NULL;
}


db_fld_t* db_fld_copy(db_fld_t* fld)
{
	int n;
	db_fld_t* newp;

	for(n = 0; fld[n].name; n++);
	n++; /* We need to copy the terminating element too */

	newp = (db_fld_t*)pkg_malloc(sizeof(db_fld_t) * n);
	if (newp == NULL) {
		ERR("db_fld: No memory left\n");
		return NULL;
	}
	memcpy(newp, fld, sizeof(db_fld_t) * n);
	if (db_fld_init(newp) < 0) goto error;
	return newp;

 error:
	if (newp) {
		db_gen_free(&newp->gen);
		pkg_free(newp);
	}
	return NULL;
}


void db_fld_free(db_fld_t* fld)
{
	db_fld_close(fld);
	pkg_free(fld);
}

/** @} */
