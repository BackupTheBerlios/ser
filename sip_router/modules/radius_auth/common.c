/*
 * $Id: common.c,v 1.3 2003/03/04 14:53:53 janakj Exp $
 *
 * Common functions needed by authorize
 * and challenge functions
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


#include "common.h"
#include "../../data_lump_rpl.h"
#include "auth_mod.h"            /* sl_reply */
#include "../../ut.h"            /* q_memchr* */


/*
 * Create a response with given code and reason phrase
 * Optionaly add new headers specified in _hdr
 */
int send_resp(struct sip_msg* _m, int _code, char* _reason, char* _hdr, int _hdr_len)
{
	struct lump_rpl* ptr;
	
	     /* Add new headers if there are any */
	if ((_hdr) && (_hdr_len)) {
		ptr = build_lump_rpl(_hdr, _hdr_len);
		add_lump_rpl(_m, ptr);
	}
	
	sl_reply(_m, (char*)_code, _reason);
	return 0;
}


/*
 * Finds specified character, that is not quoted 
 * If there is no such character, returns NULL
 *
 * PARAMS : char* _b : input buffer
 *        : char _c  : character to find
 * RETURNS: char*    : points to character found, NULL if not found
 */
char* auth_fnq(str* _b, char _c)
{
	int quoted = 0, i;
	
	if (_b->s == 0) return 0;

	for(i = 0; i < _b->len; i++) {
		if (!quoted) {
			if (_b->s[i] == '\"') quoted = 1;
			else if (_b->s[i] == _c) return _b->s + i;
		} else {
			if ((_b->s[i] == '\"') && (_b->s[i - 1] != '\\')) quoted = 0;
		}
	}
	return 0;
}


/*
 * Cut username part of a URL
 */
int auth_get_username(str* _s)
{
	char* at, *dcolon, *dc;

	     /* Find double colon, double colon
	      * separates schema and the rest of
	      * URL
	      */
	dcolon = auth_fnq(_s, ':');

	     /* No double colon found means error */
	if (!dcolon) {
		_s->len = 0;
		return -1;
	}

	     /* Skip the double colon */
	_s->len -= dcolon + 1 - _s->s;
	_s->s = dcolon + 1;

	     /* Try to find @ or another doublecolon
	      * if the URL contains also pasword, username
	      * and password will be delimited by double
	      * colon, if there is no password, @ delimites
	      * username from the rest of the URL, if there
	      * is no @, there is no username in the URL
	      */
	at = q_memchr(_s->s, '@', _s->len); /* FIXME: one pass */
	dc = q_memchr(_s->s, ':', _s->len);
	if (at) {
		     /* The double colon must be before
		      * @ to delimit username, otherwise
		      * it delimits hostname from port number
		      */
		if ((dc) && (dc < at)) {
			_s->len = dc - dcolon - 1;
			return 0;
		}
		
		_s->len = at - dcolon - 1;
		return 0;
	}

	_s->len = 0;
	return -2;
}


/*
 * Check that To header is properly parsed and if so,
 * return pointer to parsed To header.  Otherwise return NULL.
 */
inline struct to_body *get_parsed_to_body(struct sip_msg *_msg)
{
	if (!(_msg->to)) {
		LOG(L_ERR, "get_parsed_to(): Request does not have a To header\n");
		return NULL;
	}
	if (!(_msg->to->parsed) || ((struct to_body *)_msg->to->parsed)->error != PARSE_OK) {
		LOG(L_ERR, "get_parsed_to(): To header is not properly parsed\n");
		return NULL;
	}
	return (struct to_body *)(_msg->to->parsed);
}


/*
 * Check that From header is properly parsed and if so,
 * return pointer to parsed From header.  Otherwise return NULL.
 */
inline struct to_body *get_parsed_from_body(struct sip_msg *_msg)
{
	if (!(_msg->from)) {
		LOG(L_ERR, "get_parsed_from(): Request does not have a From header\n");
		return NULL;
	}
	if (!(_msg->from->parsed) || ((struct to_body *)_msg->from->parsed)->error != PARSE_OK) {
		LOG(L_ERR, "get_parsed_from(): From header is not properly parsed\n");
		return NULL;
	}
	return (struct to_body *)(_msg->from->parsed);
}
