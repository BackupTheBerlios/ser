/*
 * $Id: checks.c,v 1.10 2002/12/07 23:59:49 janakj Exp $
 *
 * Checks if To and From header fields contain the same
 * username as digest credentials
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

#include <string.h>
#include "../../str.h"
#include "../../dprint.h"               /* Debugging */
#include "../../parser/digest/digest.h" /* get_authorized_cred */
#include "../../ut.h"                   /* Handy utilities */
#include "checks.h"
#include "defs.h"
#include "common.h"


/*
 * Check if the username matches the username in credentials
 */
int is_user(struct sip_msg* _m, char* _user, char* _str2)
{
	str* s;
	struct hdr_field* h;
	auth_body_t* c;

	s = (str*)_user;

	get_authorized_cred(_m->authorization, &h);
	if (!h) {
		get_authorized_cred(_m->proxy_auth, &h);
		if (!h) {
			LOG(L_ERR, "is_user(): No authorized credentials found (error in scripts)\n");
			return -1;
		}
	}

	c = (auth_body_t*)(h->parsed);

	if (!c->digest.username.len) {
		DBG("is_user(): Username not found in credentials\n");
		return -1;
	}

	if (s->len != c->digest.username.len) {
		DBG("is_user(): Username length does not match\n");
		return -1;
	}

	if (!memcmp(s->s, c->digest.username.s, s->len)) {
		DBG("is_user(): Username matches\n");
		return 1;
	} else {
		DBG("is_user(): Username differs\n");
		return -1;
	}
}


/*
 * Check if a header field contains the same username
 * as digest credentials
 */
static inline int check_username(struct sip_msg* _m, struct hdr_field* _h)
{
	struct hdr_field* h;
	auth_body_t* c;

#ifdef USER_DOMAIN_HACK
	char* ptr;
#endif

	str user;
	int len;

	if (!_h) {
		LOG(L_ERR, "check_username(): Header Field not found\n");
		return -1;
	}

	get_authorized_cred(_m->authorization, &h);
	if (!h) {
		get_authorized_cred(_m->proxy_auth, &h);
		if (!h) {
			LOG(L_ERR, "check_username(): No authorized credentials found (error in scripts)\n");
			return -2;
		}
	}

	c = (auth_body_t*)(h->parsed);

	user.s = _h->body.s;
	user.len = _h->body.len;

	if (get_username(&user) < 0) {
		LOG(L_ERR, "check_username(): Can't extract username\n");
		return -3;
	}

	if (!user.len) return -4;

	len = c->digest.username.len;

#ifdef USER_DOMAIN_HACK
	ptr = q_memchr(c->digest.username.s, '@', len);
	if (ptr) {
		len = ptr - c->digest.username.s;
	}
#endif

	if (user.len == len) {
		if (!memcmp(user.s, c->digest.username.s, len)) {
			DBG("check_username(): Username is same\n");
			return 1;
		}
	}
	
	DBG("check_username(): Username is different\n");
	return -5;
}


/*
 * Check username part in To header field
 */
int check_to(struct sip_msg* _m, char* _s1, char* _s2)
{
	if (!_m->to && ((parse_headers(_m, HDR_TO, 0) == -1) || (!_m->to))) {
		LOG(L_ERR, "check_to(): Error while parsing To header field\n");
		return -1;
	}
	return check_username(_m, _m->to);
}


/*
 * Check username part in From header field
 */
int check_from(struct sip_msg* _m, char* _s1, char* _s2)
{
	if (!_m->from && ((parse_headers(_m, HDR_FROM, 0) == -1) || (!_m->from))) {
		LOG(L_ERR, "check_from(): Error while parsing From header field\n");
		return -1;
	}
	return check_username(_m, _m->from);
}
