/*
 * $Id: api.c,v 1.14 2006/04/24 18:16:59 janakj Exp $
 *
 * Digest Authentication Module
 *
 * Copyright (C) 2001-2003 FhG Fokus
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
#include "api.h"
#include "../../dprint.h"
#include "../../parser/digest/digest.h"
#include "../../sr_module.h"
#include "auth_mod.h"
#include "nonce.h"
#include "common.h"


/*
 * Find credentials with given realm in a SIP message header
 */
static inline int find_credentials(struct sip_msg* msg, str* realm,
				   hdr_types_t hftype, struct hdr_field** hdr)
{
	struct hdr_field** hook, *ptr, *prev;
	hdr_flags_t hdr_flags;
	int res;
	str* r;

	     /*
	      * Determine if we should use WWW-Authorization or
	      * Proxy-Authorization header fields, this parameter
	      * is set in www_authorize and proxy_authorize
	      */
	switch(hftype) {
	case HDR_AUTHORIZATION_T: 
		hook = &(msg->authorization);
		hdr_flags=HDR_AUTHORIZATION_F;
		break;
	case HDR_PROXYAUTH_T:
		hook = &(msg->proxy_auth);
		hdr_flags=HDR_PROXYAUTH_F;
		break;
	default:				
		hook = &(msg->authorization);
		hdr_flags=HDR_T2F(hftype);
		break;
	}

	     /*
	      * If the credentials haven't been parsed yet, do it now
	      */
	if (*hook == 0) {
		     /* No credentials parsed yet */
		if (parse_headers(msg, hdr_flags, 0) == -1) {
			LOG(L_ERR, "auth:find_credentials: Error while parsing headers\n");
			return -1;
		}
	}

	ptr = *hook;

	     /*
	      * Iterate through the credentials in the message and
	      * find credentials with given realm
	      */
	while(ptr) {
		res = parse_credentials(ptr);
		if (res < 0) {
			LOG(L_ERR, "auth:find_credentials: Error while parsing credentials\n");
			return (res == -1) ? -2 : -3;
		} else if (res == 0) {
			r = &(((auth_body_t*)(ptr->parsed))->digest.realm);

			if (r->len == realm->len) {
				if (!strncasecmp(realm->s, r->s, r->len)) {
					*hdr = ptr;
					return 0;
				}
			}
		}

		prev = ptr;
		if (parse_headers(msg, hdr_flags, 1) == -1) {
			LOG(L_ERR, "auth:find_credentials: Error while parsing headers\n");
			return -4;
		} else {
			if (prev != msg->last_header) {
				if (msg->last_header->type == hftype) ptr = msg->last_header;
				else break;
			} else break;
		}
	}
	
	     /*
	      * Credentials with given realm not found
	      */
	return 1;
}


/*
 * Purpose of this function is to find credentials with given realm,
 * do sanity check, validate credential correctness and determine if
 * we should really authenticate (there must be no authentication for
 * ACK and CANCEL
 */
auth_result_t pre_auth(struct sip_msg* msg, str* realm, hdr_types_t hftype,
						struct hdr_field**  hdr)
{
	int ret;
	auth_body_t* c;
	static str prack = STR_STATIC_INIT("PRACK");

	     /* ACK and CANCEL must be always authenticated, there is
	      * no way how to challenge ACK and CANCEL cannot be
	      * challenged because it must have the same CSeq as
	      * the request to be canceled
	      */

	if ((msg->REQ_METHOD == METHOD_ACK) ||  (msg->REQ_METHOD == METHOD_CANCEL)) return AUTHENTICATED;
	     /* PRACK is also not authenticated */
	if ((msg->REQ_METHOD == METHOD_OTHER)) {
		if (msg->first_line.u.request.method.len == prack.len &&
		    !memcmp(msg->first_line.u.request.method.s, prack.s, prack.len))
			return AUTHENTICATED;
	}

	if (realm->len == 0) {
		if (get_realm(msg, hftype, realm) < 0) {
			LOG(L_ERR, "auth:pre_auth: Error while extracting realm\n");
			if (send_resp(msg, 400, MESSAGE_400, 0, 0) == -1) {
				LOG(L_ERR, "auth:pre_auth: Error while sending 400 reply\n");
			}
			return ERROR;
		}
	}

	     /* Try to find credentials with corresponding realm
	      * in the message, parse them and return pointer to
	      * parsed structure
	      */
	ret = find_credentials(msg, realm, hftype, hdr);
	if (ret < 0) {
		LOG(L_ERR, "auth:pre_auth: Error while looking for credentials\n");
		if (send_resp(msg, (ret == -2) ? 500 : 400, 
			      (ret == -2) ? MESSAGE_500 : MESSAGE_400, 0, 0) == -1) {
			LOG(L_ERR, "auth:pre_auth: Error while sending 400 reply\n");
		}
		return ERROR;
	} else if (ret > 0) {
		DBG("auth:pre_auth: Credentials with given realm not found\n");
		return NOT_AUTHENTICATED;
	}

	     /* Pointer to the parsed credentials */
	c = (auth_body_t*)((*hdr)->parsed);

	     /* Check credentials correctness here */
	if (check_dig_cred(&(c->digest)) != E_DIG_OK) {
		LOG(L_ERR, "auth:pre_auth: Credentials received are not filled properly\n");
		if (send_resp(msg, 400, MESSAGE_400, 0, 0) == -1) {
			LOG(L_ERR, "auth:pre_auth: Error while sending 400 reply\n");
		}
		return ERROR;
	}

	if (check_nonce(&c->digest.nonce, &secret, msg) != 0) {
		DBG("auth:pre_auth: Invalid nonce value received\n");
		return NOT_AUTHENTICATED;
	}

	return DO_AUTHENTICATION;
}


/*
 * Purpose of this function is to do post authentication steps like
 * marking authorized credentials and so on.
 */
auth_result_t post_auth(struct sip_msg* msg, struct hdr_field* hdr)
{
	int res = AUTHENTICATED;
	auth_body_t* c;

	c = (auth_body_t*)((hdr)->parsed);

	if (is_nonce_stale(&c->digest.nonce)) {
		if ((msg->REQ_METHOD == METHOD_ACK) || 
		    (msg->REQ_METHOD == METHOD_CANCEL)) {
			     /* Method is ACK or CANCEL, we must accept stale
			      * nonces because there is no way how to challenge
			      * with new nonce (ACK has no response associated 
			      * and CANCEL must have the same CSeq as the request 
			      * to be canceled)
			      */
		} else {
			DBG("auth:post_auth: Response is OK, but nonce is stale\n");
			c->stale = 1;
			res = NOT_AUTHENTICATED;
		}
	}

	if (mark_authorized_cred(msg, hdr) < 0) {
		LOG(L_ERR, "auth:post_auth: Error while marking parsed credentials\n");
		if (send_resp(msg, 500, MESSAGE_500, 0, 0) == -1) {
			LOG(L_ERR, "auth:post_auth: Error while sending 500 reply\n");
		}
		res = ERROR;
	}

	return res;
}


int bind_auth(auth_api_t* api)
{
	if (!api) {
		LOG(L_ERR, "bind_auth: Invalid parameter value\n");
		return -1;
	}

	api->pre_auth = pre_auth;
	api->post_auth = post_auth;

	return 0;
}
