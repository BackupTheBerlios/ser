/*
 * $Id: checks.c,v 1.11 2002/12/08 17:18:56 janakj Exp $
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
#include "../../parser/parse_from.h"
#include "../../parser/parse_uri.h"
#include "../../ut.h"                   /* Handy utilities */
#include "../../db/db.h"                /* Database API */
#include "auth_mod.h"
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


/*
 * Check if realm is local
 */
static int is_realm_local(char* _table, str* _host)
{
	db_key_t keys[] = {realm_realm_col};
	db_val_t vals[1];
	db_key_t cols[] = {realm_realm_col};
	db_res_t* res;

	if (db_use_table(db_handle, _table) < 0) {
		LOG(L_ERR, "is_realm_local(): Error while trying to use realm table\n");
	}

	VAL_TYPE(vals) = DB_STR;
	VAL_NULL(vals) = 0;
    
	VAL_STR(vals).s = _host->s;
	VAL_STR(vals).len = _host->len;

	if (db_query(db_handle, keys, 0, vals, cols, 1, 1, 0, &res) < 0) {
		LOG(L_ERR, "is_realm_local(): Error while querying database\n");
		return -1;
	}

	if (RES_ROW_N(res) == 0) {
		DBG("is_realm_local(): Realm \'%.*s\' is not local\n", _host->len, _host->s);
		db_free_query(db_handle, res);
		return -1;
	} else {
		DBG("is_realm_local(): Realm \'%.*s\' is local\n", _host->len, _host->s);
		db_free_query(db_handle, res);
		return 1;
	}
}


/*
 * Check if host in From uri is local
 */
int is_from_local(struct sip_msg* _msg, char* _table, char* _s2)
{
	struct sip_uri uri;

	if (parse_from_header(_msg) < 0) {
		LOG(L_ERR, "is_from_local(): Can't find From header\n");
		return -1;
	}
	
	if (parse_uri(get_from(_msg)->uri.s, get_from(_msg)->uri.len, &uri) < 0) {
		LOG(L_ERR, "is_from_local(): Error while parsing From uri\n");
		return -1;
	}
	
	return is_realm_local(_table, &(uri.host));
}


/*
 * Check if host in Request URI is local
 */
int is_uri_host_local(struct sip_msg* _msg, char* _table, char* _s2)
{
	if (parse_sip_msg_uri(_msg) < 0) {
		LOG(L_ERR, "is_uri_host_local(): Error while parsing URI\n");
		return -1;
	}

	return is_realm_local(_table, &(_msg->parsed_uri.host));
}


/*
 * Check if uri belongs to a local user
 */
int does_uri_exist(struct sip_msg* _msg, char* _table, char* _s2)
{
	db_key_t keys[2], cols[1];
	db_val_t vals[2];
	db_res_t* res;

	if (parse_sip_msg_uri(_msg) < 0) {
		LOG(L_ERR, "does_uri_exist(): Bad URI\n");
		return -1;
	}

	if (db_use_table(db_handle, _table) < 0) {
		LOG(L_ERR, "does_uri_exist(): Error while trying to use table \'%s\'\n", _table);
		return -1;
	}

	keys[0] = realm_column;
	keys[1] = user_column;
	cols[0] = user_column;

	VAL_TYPE(vals) = VAL_TYPE(vals + 1) = DB_STR;
	VAL_NULL(vals) = VAL_NULL(vals + 1) = 0;
	VAL_STR(vals) = _msg->parsed_uri.host;
	VAL_STR(vals + 1) = _msg->parsed_uri.user;

	if (db_query(db_handle, keys, 0, vals, cols, 2, 1, 0, &res) < 0) {
		LOG(L_ERR, "is_local(): Error while querying database\n");
		return -1;
	}
	
	if (RES_ROW_N(res) == 0) {
		DBG("does_uri_exist(): User \'%.*s@%.*s\' doesn't exist\n",
			_msg->parsed_uri.user.len, _msg->parsed_uri.user.s,
			_msg->parsed_uri.host.len, _msg->parsed_uri.host.s
		    );
		db_free_query(db_handle, res);
		return -1;
	} else {
		DBG("does_uri_exist(): User \'%.*s@%.*s\' does exist\n",
			_msg->parsed_uri.user.len, _msg->parsed_uri.user.s,
			_msg->parsed_uri.host.len, _msg->parsed_uri.host.s
		    );
		db_free_query(db_handle, res);
		return 1;
	}
}
