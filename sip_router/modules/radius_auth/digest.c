/* 
 * Radius digest Authorize method.
 * @author Stelios Sidiroglou-Douskos <ssi@fokus.gmd.de>
 * $Id: digest.c,v 1.3 2003/03/04 14:53:53 janakj Exp $
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


#include <radiusclient.h>
#include "ser_radius.h"
#include <stdlib.h>
#include "../../config.h"
#include "digest.h"
#include "../../str.h"
#include "../../parser/digest/digest_parser.h"
#include "../../ut.h"
#include <string.h>
#include "../../dprint.h"

/*
 * This function creates and submits radius authentication request as per
 * draft-sterman-aaa-sip-00.txt.  In addition, _user parameter is included
 * in the request as value of a SER specific attribute type SIP-URI-User,
 * which can be be used as a check item in the request.  Service type of
 * the request is Authenticate-Only.
 */
int radius_authorize_sterman(dig_cred_t * cred, str* _method, str* _user) 
{
	char            msg[4096];
	VALUE_PAIR      *send, *received;
	UINT4           service;
	VALUE_PAIR 	*vp;    
	str		method, user, user_name;

	send = NULL;
	received = NULL;

	method.s = _method->s;
	method.len = _method->len;

	user.s = _user->s;
	user.len = _user->len;

	/*
	 * Add all the user digest parameters according to the qop defined.
	 * Most devices tested only offer support for the simplest digest.
	 */

	if (q_memchr(cred->username.s, '@', cred->username.len)) {
		if (rc_avpair_add(&send, PW_USER_NAME, cred->username.s, cred->username.len) == NULL)
			rc_avpair_free(send);
			return -1;
	} else {
		user_name.len = cred->username.len + cred->realm.len + 1;
		user_name.s = malloc(user_name.len);
		if (!user_name.s) {
			return -1;
		}
		strncpy(user_name.s, cred->username.s, cred->username.len);
		user_name.s[cred->username.len] = '@';
		strncpy(user_name.s + cred->username.len + 1, cred->realm.s, cred->realm.len);
		if (rc_avpair_add(&send, PW_USER_NAME, user_name.s, user_name.len) == NULL) {
			free(user_name.s);
			rc_avpair_free(send);
			return -1;
		}
		free(user_name.s);
	}

	if (rc_avpair_add(&send, PW_DIGEST_USER_NAME, cred->username.s, cred->username.len) == NULL) {
		rc_avpair_free(send);
		return -1;
	}

	if (rc_avpair_add(&send, PW_DIGEST_REALM, cred->realm.s, cred->realm.len) == NULL) {
		rc_avpair_free(send);
		return -1;
	}
	if (rc_avpair_add(&send, PW_DIGEST_NONCE, cred->nonce.s, cred->nonce.len) == NULL) {
		rc_avpair_free(send);
		return -1;
	}
	
	if (rc_avpair_add(&send, PW_DIGEST_URI, cred->uri.s, cred->uri.len) == NULL) {
		rc_avpair_free(send);
		return -1;
	}
	if (rc_avpair_add(&send, PW_DIGEST_METHOD, method.s, method.len) == NULL) {
		rc_avpair_free(send);
		return -1;
	}
	
	/* 
	 * Add the additional authentication fields according to the QOP.
	 */
	if (cred->qop.qop_parsed == QOP_AUTH) {
		if (rc_avpair_add(&send, PW_DIGEST_QOP, "auth", 4) == NULL) {
			rc_avpair_free(send);
			return -1;
		}
		if (rc_avpair_add(&send, PW_DIGEST_NONCE_COUNT, cred->nc.s, cred->nc.len) == NULL) {
			rc_avpair_free(send);
			return -1;
		}
		if (rc_avpair_add(&send, PW_DIGEST_CNONCE, cred->cnonce.s, cred->cnonce.len) == NULL) {
			rc_avpair_free(send);
			return -1;
		}
	} else if (cred->qop.qop_parsed == QOP_AUTHINT) {
		if (rc_avpair_add(&send, PW_DIGEST_QOP, "auth-int", 8) == NULL) {
			rc_avpair_free(send);
			return -1;
		}
		if (rc_avpair_add(&send, PW_DIGEST_NONCE_COUNT, cred->nc.s, cred->nc.len) == NULL) {
			rc_avpair_free(send);
			return -1;
		}
		if (rc_avpair_add(&send, PW_DIGEST_CNONCE, cred->cnonce.s, cred->cnonce.len) == NULL) {
			rc_avpair_free(send);
			return -1;
		}
		if (rc_avpair_add(&send, PW_DIGEST_BODY_DIGEST, cred->opaque.s, cred->opaque.len) == NULL) {
			rc_avpair_free(send);
			return -1;
		}
		
	} else  {
		/* send nothing for qop == "" */
	}

	/*
	 * Now put everything place all the previous attributes into the
	 * PW_DIGEST_ATTRIBUTES
	 */
	
	/*
	 *  Fix up Digest-Attributes issues see draft-sterman-aaa-sip-00
	 */
	for (vp = send; vp != NULL; vp = vp->next) {
		switch (vp->attribute) {
  		default:
			break;

			/* Fall thru the know values */
		case PW_DIGEST_REALM:
		case PW_DIGEST_NONCE:
		case PW_DIGEST_METHOD:
		case PW_DIGEST_URI:
		case PW_DIGEST_QOP:
		case PW_DIGEST_ALGORITHM:
		case PW_DIGEST_BODY_DIGEST:
		case PW_DIGEST_CNONCE:
		case PW_DIGEST_NONCE_COUNT:
		case PW_DIGEST_USER_NAME:
	
			/* overlapping! */
			memmove(&vp->strvalue[2], &vp->strvalue[0], vp->lvalue);
			vp->strvalue[0] = vp->attribute - PW_DIGEST_REALM + 1;
			vp->lvalue += 2;
			vp->strvalue[1] = vp->lvalue;
			vp->attribute = PW_DIGEST_ATTRIBUTES;
			break;
		}
	}

	/* Add the response... What to calculate against... */
	if (rc_avpair_add(&send, PW_DIGEST_RESPONSE, cred->response.s, cred->response.len) == NULL) {
		rc_avpair_free(send);
		return -1;
	}

	/* Indicate the service type, Authenticate only in our case */
	service = PW_AUTHENTICATE_ONLY;
	if (rc_avpair_add(&send, PW_SERVICE_TYPE, &service, 0) == NULL) {
		DBG("radius_authorize() Error adding service type\n");
		rc_avpair_free(send);
	 	return -1;
	}

	/* Add SIP URI as a check item */
	if (rc_avpair_add(&send, PW_SIP_URI_USER, user.s, user.len) == NULL) {
		DBG("radius_authorize() Error adding SIP URI\n");
		rc_avpair_free(send);
	 	return -1;  	
	}
       
	/* Send request */
	if (rc_auth(SIP_PORT, send, &received, msg) == OK_RC) {
		DBG("radius_authorize_sterman(): Success\n");
		rc_avpair_free(send);
		rc_avpair_free(received);
		return 1;
	} else {
		DBG("radius_authorize_sterman(): Failure\n");
		rc_avpair_free(send);
		rc_avpair_free(received);
		return -1;
	}
}
