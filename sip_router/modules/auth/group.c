/*
 * $Id: group.c,v 1.12 2002/12/07 23:59:49 janakj Exp $
 *
 * Checks if a username matche those in digest credentials
 * or is member of a group
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
#include "../../dprint.h"               /* Logging */
#include "../../db/db.h"                /* Generic database API */
#include "../../parser/digest/digest.h" /* get_authorized_cred */
#include "../../parser/hf.h"            /* Header Field types */
#include "../../parser/parse_from.h"    /* From parser */
#include "group.h"
#include "auth_mod.h"                   /* Module parameters */
#include "common.h"


/*
 * Check if the user specified in credentials is a member
 * of given group
 */
int is_in_group(struct sip_msg* _m, char* _group, char* _str2)
{
	db_key_t keys[] = {grp_user_col, grp_grp_col};
	db_val_t vals[2];
	db_key_t col[] = {grp_grp_col};
	db_res_t* res;
	struct hdr_field* h;
	auth_body_t* c;

	get_authorized_cred(_m->authorization, &h);
	if (!h) {
		get_authorized_cred(_m->proxy_auth, &h);
		if (!h) {
			LOG(L_ERR, "is_in_group(): No authorized credentials found (error in scripts)\n");
			return -1;
		}
	}

	c = (auth_body_t*)(h->parsed);

	VAL_TYPE(vals) = VAL_TYPE(vals + 1) = DB_STR;
	VAL_NULL(vals) = VAL_NULL(vals + 1) = 0;

	VAL_STR(vals) = c->digest.username;
	VAL_STR(vals + 1) = *((str*)_group);
	
	db_use_table(db_handle, grp_table);
	if (db_query(db_handle, keys, 0, vals, col, 2, 1, 0, &res) < 0) {
		LOG(L_ERR, "is_in_group(): Error while querying database\n");
		return -1;
	}
	
	if (RES_ROW_N(res) == 0) {
		DBG("is_in_group(): User \'%.*s\' is not in group \'%.*s\'\n", 
		    c->digest.username.len, c->digest.username.s,
		    ((str*)_group)->len, ((str*)_group)->s);
		db_free_query(db_handle, res);
		return -1;
	} else {
		DBG("is_in_group(): User \'%.*s\' is member of group \'%.*s\'\n", 
		    c->digest.username.len, c->digest.username.s,
		    ((str*)_group)->len, ((str*)_group)->s);
		db_free_query(db_handle, res);
		return 1;
	}
}


/*
 * Extract username from Request-URI
 */
static inline int get_request_user(struct sip_msg* _m, str* _s)
{
	if (_m->new_uri.s) {
		_s->s = _m->new_uri.s;
		_s->len = _m->new_uri.len;
	} else {
		_s->s = _m->first_line.u.request.uri.s;
		_s->len = _m->first_line.u.request.uri.len;
	}
	if (get_username(_s) < 0) {
		LOG(L_ERR, "get_request_user(): Error while extracting username\n");
		return -1;
	}
	return 0;
}


/*
 * Extract username from To header field
 */
static inline int get_to_user(struct sip_msg* _m, str* _s)
{
	if (!_m->to && ((parse_headers(_m, HDR_TO, 0) == -1) || (!_m->to))) {
		LOG(L_ERR, "is_user_in(): Can't get To header field\n");
		return -1;
	}
	
	_s->s = ((struct to_body*)_m->to->parsed)->uri.s;
	_s->len = ((struct to_body*)_m->to->parsed)->uri.len;

	if (get_username(_s) < 0) {
		LOG(L_ERR, "get_to_user(): Error while extracting username\n");
		return -2;
	}
	return 0;
}


/*
 * Extract username from From header field
 */
static inline int get_from_user(struct sip_msg* _m, str* _s)
{
	if (parse_from_header(_m) < 0) {
		LOG(L_ERR, "is_user_in(): Error while parsing From body\n");
		return -1;
	}
	
	_s->s = ((struct to_body*)_m->from->parsed)->uri.s;
	_s->len = ((struct to_body*)_m->from->parsed)->uri.len;

	if (get_username(_s) < 0) {
		LOG(L_ERR, "is_user_in(): Error while extracting username\n");
		return -2;
	}

	return 0;
}


/*
 * Extract username from digest credentials
 */
static inline int get_cred_user(struct sip_msg* _m, str* _s)
{
	struct hdr_field* h;
	auth_body_t* c;
	
	get_authorized_cred(_m->authorization, &h);
	if (!h) {
		get_authorized_cred(_m->proxy_auth, &h);
		if (!h) {
			LOG(L_ERR, "is_user_in(): No authorized credentials found (error in scripts)\n");
			return -1;
		}
	}
	
	c = (auth_body_t*)(h->parsed);

	_s->s = c->digest.username.s;
	_s->len = c->digest.username.len;

	return 0;
}


/*
 * Check if username in specified header field is in a table
 */
int is_user_in(struct sip_msg* _msg, char* _hf, char* _grp)
{
	db_key_t keys[] = {grp_user_col, grp_grp_col};
	db_val_t vals[2];
	db_key_t col[1] = {grp_grp_col};
	db_res_t* res;
	str user;

	switch((int)_hf) {
	case 1: /* Request-URI */
		if (get_request_user(_msg, &user) < 0) {
			LOG(L_ERR, "is_user_in(): Error while obtaining username from Request-URI\n");
			return -1;
		}
		break;

	case 2: /* To */
		if (get_to_user(_msg, &user) < 0) {
			LOG(L_ERR, "is_user_in(): Error while extracting To username\n");
			return -2;
		}
		break;

	case 3: /* From */
		if (get_from_user(_msg, &user) < 0) {
			LOG(L_ERR, "is_user_in(): Error while extracting From username\n");
			return -3;
		}
		break;

	case 4: /* Credentials */
		if (get_cred_user(_msg, &user) < 0) {
			LOG(L_ERR, "is_user_in(): Error while extracting digest username\n");
			return -4;
		}
		break;
	}

	VAL_TYPE(vals) = VAL_TYPE(vals + 1) = DB_STR;
	VAL_NULL(vals) = VAL_NULL(vals + 1) = 0;
	VAL_STR(vals) = user;

	VAL_STR(vals + 1) = *((str*)_grp);
	
	db_use_table(db_handle, grp_table);
	if (db_query(db_handle, keys, 0, vals, col, 2, 1, 0, &res) < 0) {
		LOG(L_ERR, "is_user_in(): Error while querying database\n");
		return -5;
	}
	
	if (RES_ROW_N(res) == 0) {
		DBG("is_user_in(): User \'%.*s\' is not in group \'%.*s\'\n", 
		    user.len, user.s,
		    ((str*)_grp)->len, ((str*)_grp)->s);
		db_free_query(db_handle, res);
		return -6;
	} else {
		DBG("is_user(): User \'%.*s\' is in table \'%.*s\'\n", 
		    user.len, user.s,
		    ((str*)_grp)->len, ((str*)_grp)->s);
		db_free_query(db_handle, res);
		return 1;
	}
}
