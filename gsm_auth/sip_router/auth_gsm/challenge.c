/*
 * $Id: challenge.c,v 1.1 2003/12/09 12:43:22 dcm Exp $
 *
 * Challenge related functions
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

/**
 * History:
 * -------
 *  2003-07-03  first version (dcm)
 */


#include "../../data_lump.h"
#include "../../mem/mem.h"
#include "../../parser/digest/digest.h"
#include "../../parser/parse_from.h"
#include "../../parser/parse_uri.h"
#include "../../data_lump_rpl.h"

#include "auth_gsm.h"

/*
 * proxy_challenge function sends this reply
 */
#define MESSAGE_407          "Proxy Authentication Required"
#define PROXY_AUTH_CHALLENGE "Proxy-Authenticate"


/*
 * www_challenge function send this reply
 */
#define MESSAGE_401        "Unauthorized"
#define WWW_AUTH_CHALLENGE "WWW-Authenticate"

#define MESSAGE_400 "Bad Request"
#define MESSAGE_500 "Server Internal Error"


#define GSM_STR			": GSM "
#define GSM_STR_LEN		(sizeof(GSM_STR)-1)
#define GSM_REALM		"realm=\""
#define GSM_REALM_LEN	(sizeof(GSM_REALM)-1)
#define GSM_NONCE		"\", nonce=\""
#define GSM_NONCE_LEN	(sizeof(GSM_NONCE)-1)


/* 
 * Return parsed To or From, host part of the parsed uri is realm
 */
int gsm_get_realm(struct sip_msg* _m, int _hftype, struct sip_uri* _u)
{
	str uri;

	if ((REQ_LINE(_m).method.len == 8) 
	    && !memcmp(REQ_LINE(_m).method.s, "REGISTER", 8) 
	    && (_hftype == HDR_AUTHORIZATION)
	   ) {
		if (!_m->to && ((parse_headers(_m, HDR_TO, 0) == -1) || (!_m->to)))
		{
			LOG(L_ERR, "auth_gsm:get_realm: Error while parsing headers\n");
			return -1;
		}
		
		     /* Body of To header field is parsed automatically */
		uri = get_to(_m)->uri; 
	} else {
		if (parse_from_header(_m) < 0) {
			LOG(L_ERR, "auth_gsm:get_realm: Error while parsing headers!\n");
			return -2;
		}

		uri = get_from(_m)->uri;
	}

	if (parse_uri(uri.s, uri.len, _u) < 0) {
		LOG(L_ERR, "auth_gsm:get_realm: Error while parsing URI\n");
		return -3;
	}
	
	return 0;
}


/*
 * Create a response with given code and reason phrase
 * Optionaly add new headers specified in _hdr
 */
int gsm_send_resp(struct sip_msg* _m, int _code, char* _reason,
		char* _hdr, int _hdr_len)
{
	struct lump_rpl* ptr;
	
	     /* Add new headers if there are any */
	if ((_hdr) && (_hdr_len)) {
		ptr = build_lump_rpl(_hdr, _hdr_len);
		add_lump_rpl(_m, ptr);
	}
	
	return sl_reply(_m, (char*)_code, _reason);
}

/*
 * Create {WWW,Proxy}-Authenticate header field
 */
static inline char *gsm_build_auth_hf(char* _chdr, str* _realm, char *_hbuf,
		int _hlen, int* _len)
{
	
	int chdr_len;
	char *hf=NULL, *p;

	*_len = chdr_len = strlen(_chdr);
	*_len += GSM_STR_LEN + CRLF_LEN + GSM_REALM_LEN + _realm->len + 1;
	if(_hbuf)
		*_len += GSM_NONCE_LEN + _hlen;
	

	p = hf = pkg_malloc(*_len+1);
	if (!hf) 
	{
		LOG(L_ERR, "auth_gsm:build_auth_hf:ERROR - no memory\n");
		*_len=0;
		return 0;
	}
	memcpy(p, _chdr, chdr_len); p+=chdr_len;
	memcpy(p, GSM_STR, GSM_STR_LEN);p+=GSM_STR_LEN;
	memcpy(p, GSM_REALM, GSM_REALM_LEN);p+=GSM_REALM_LEN;
	memcpy(p, _realm->s, _realm->len);p+=_realm->len;
	if(_hbuf)
	{
		memcpy(p, GSM_NONCE, GSM_NONCE_LEN);
		p+=GSM_NONCE_LEN;
		memcpy(p, _hbuf, _hlen);p+=_hlen;
	}
	
	*p++ = '"';
	memcpy(p, CRLF, CRLF_LEN ); p+=CRLF_LEN;
	*p=0; /* zero terminator, just in case */


	DBG("auth_gsm:build_auth_hf: auth hdr [%s]\n", hf);
	return hf;
}

/*
 * Create and send a challenge
 */
static inline int gsm_challenge(struct sip_msg* _msg, str* _realm, int _code,
		char* _message, char* _chdr, char *_hbuf, int _hlen)
{
	int auth_hf_len;
	char *auth_hf;
	int ret, hftype = 0; /* Makes gcc happy */
	struct sip_uri uri;
	
	switch(_code)
	{
	case 401: 
		hftype = HDR_AUTHORIZATION;
		break;
	case 407: 
		hftype = HDR_PROXYAUTH;
		break;
	}


	if (gsm_get_realm(_msg, hftype, &uri) < 0)
	{
		LOG(L_ERR, "auth_gsm:gsm_challenge: Error while extracting URI\n");
		if (gsm_send_resp(_msg, 400, MESSAGE_400, 0, 0) == -1) 
		{
			LOG(L_ERR, "auth_gsm:gsm_challenge: Error while sending response\n");
			return -1;
		}
		return 0;
	}
	

	auth_hf = gsm_build_auth_hf(_chdr, _realm, _hbuf, _hlen, &auth_hf_len);
	if (!auth_hf)
	{
		LOG(L_ERR, "ERROR:auth_gsm:gsm_challenge: no mem w/cred\n");
		return -1;
	}
	
	ret = gsm_send_resp(_msg, _code, _message, auth_hf, auth_hf_len);
	if (auth_hf)
		pkg_free(auth_hf);
	if (ret == -1)
	{
		LOG(L_ERR, "auth_gsm:gsm_challenge: Error while sending response\n");
		return -1;
	}
	
	return 0;
}


/*
 * Challenge a user to send credentials using WWW-Authorize header field
 */
int gsm_www_challenge(struct sip_msg* msg, str* _realm, char *hbuf, int hlen)
{
	return gsm_challenge(msg, _realm, 401, MESSAGE_401, WWW_AUTH_CHALLENGE,
			hbuf, hlen);
}


/*
 * Challenge a user to send credentials using Proxy-Authorize header field
 */
int gsm_proxy_challenge(struct sip_msg* msg, str* _realm, char *hbuf, int hlen)
{
	return gsm_challenge(msg, _realm, 407, MESSAGE_407, PROXY_AUTH_CHALLENGE,
			hbuf, hlen);
}

