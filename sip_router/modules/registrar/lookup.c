/*
 * $Id: lookup.c,v 1.9 2003/01/14 23:32:52 janakj Exp $
 *
 * Lookup contacts in usrloc
 *
 * Copyright (C) 2001-2003 Fhg Fokus
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


#include "lookup.h"
#include <string.h>
#include "../../dset.h"
#include "../../str.h"
#include "../../config.h"
#include "../../action.h"
#include "../usrloc/usrloc.h"
#include "common.h"
#include "regtime.h"
#include "reg_mod.h"


/*
 * Rewrite Request-URI
 */
static inline int rwrite(struct sip_msg* _m, str* _s)
{
	char buffer[MAX_URI_SIZE];
	struct action act;
	
	if (_s->len > MAX_URI_SIZE - 1) {
		LOG(L_ERR, "rwrite(): URI too long\n");
		return -1;
	}
	
	memcpy(buffer, _s->s, _s->len);
	buffer[_s->len] = '\0';
	
	DBG("rwrite(): Rewriting Request-URI with \'%s\'\n", buffer);
	act.type = SET_URI_T;
	act.p1_type = STRING_ST;
	act.p1.string = buffer;
	act.next = 0;
	
	if (do_action(&act, _m) < 0) {
		LOG(L_ERR, "rwrite(): Error in do_action\n");
		return -1;
	}
	return 0;
}


/*
 * Lookup contact in the database and rewrite Request-URI
 */
int lookup(struct sip_msg* _m, char* _t, char* _s)
{
	urecord_t* r;
	str aor, uri;
	ucontact_t* ptr;
	int res;
	
	if (_m->new_uri.s) uri = _m->new_uri;
	else uri = _m->first_line.u.request.uri;
	
	if (extract_aor(&uri, &aor) < 0) {
		LOG(L_ERR, "lookup(): Error while extracting address of record\n");
		return -1;
	}
	
	get_act_time();

	ul_lock_udomain((udomain_t*)_t);
	res = ul_get_urecord((udomain_t*)_t, &aor, &r);
	if (res < 0) {
		LOG(L_ERR, "lookup(): Error while querying usrloc\n");
		ul_unlock_udomain((udomain_t*)_t);
		return -2;
	}
	
	if (res > 0) {
		DBG("lookup(): \'%.*s\' Not found in usrloc\n", aor.len, aor.s);
		ul_unlock_udomain((udomain_t*)_t);
		return -3;
	}

	ptr = r->contacts;
	while ((ptr) && (ptr->expires <= act_time)) ptr = ptr->next;
	
	if (ptr) {
		if (rwrite(_m, &ptr->c) < 0) {
			LOG(L_ERR, "lookup(): Unable to rewrite Request-URI\n");
			ul_unlock_udomain((udomain_t*)_t);
			return -4;
		}
		ptr = ptr->next;
	} else {
		     /* All contacts expired */
		ul_unlock_udomain((udomain_t*)_t);
		return -5;
	}
	
	     /* Append branches if enabled */
	if (!append_branches) goto skip;

	while(ptr) {
		if (ptr->expires > act_time) {
			if (append_branch(_m, ptr->c.s, ptr->c.len) == -1) {
				LOG(L_ERR, "lookup(): Error while appending a branch\n");
				ul_unlock_udomain((udomain_t*)_t);
				return -6;
			}
		} 
		ptr = ptr->next;
	}
	
 skip:
	ul_unlock_udomain((udomain_t*)_t);
	return 1;
}
