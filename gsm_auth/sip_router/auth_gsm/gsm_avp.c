/* 
 * $Id: gsm_avp.c,v 1.2 2004/09/17 10:38:58 dcm Exp $
 *
 * GSM Authentication - Radius support
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
 *
 * History:
 * -------
 * 2003-07-03: first version (dcm)
 */


#include <string.h>
#include "../../mem/mem.h"
#include "../../dprint.h"
#include "../auth/api.h"
#include "gsm_radius.h"
#include "gsm_avp.h"
#include "auth_gsm.h"
#include <radiusclient.h>

#define VENDOR_IPGSM 7879
#define IPGSM_AVPCODE(x) ((VENDOR_IPGSM << 16) | (x))
/*
 * This function creates and submits radius authentication request
 * Service type of the request is Authenticate-Only.
 */
int gsm_authorize_rad(dig_cred_t* _cred, str* _user, str *_nonce)
{
	static char msg[4096];
	VALUE_PAIR *send, *received, *vp;
	UINT4 service;
	int ret;

	send = received = 0;

	if (!(_cred && _user)) {
		LOG(L_ERR, "gsm_authorize_rad: Invalid parameter value\n");
		return -1;
	}

	/*
	 * Add all the user digest parameters according to the qop defined.
	 * Most devices tested only offer support for the simplest digest.
	 */

	if (!rc_avpair_add(rh, &send, PW_USER_NAME, _cred->username.user.s,
				_cred->username.user.len, 0))
	{
		rc_avpair_free(send);
		return -2;
	}
	
	if (!rc_avpair_add(rh, &send, PW_USER_REALM,_cred->realm.s,
			_cred->realm.len, 0))
	{
		rc_avpair_free(send);
		return -6;
	}
	
	/* Add the response... What to calculate against... */
	if(_cred->response.s)
	{
		if (!rc_avpair_add(rh, &send, PW_CHAP_CHALLENGE, _cred->nonce.s,
					_cred->nonce.len, 0)) {
			rc_avpair_free(send);
			return -7;
		}
		if (!rc_avpair_add(rh, &send, PW_USER_PASSWORD, _cred->response.s,
				_cred->response.len, 0)) {
			rc_avpair_free(send);
			return -17;
		}
	}

	/* Indicate the service type, Authenticate only in our case */
	service = service_type;
	if (!rc_avpair_add(rh, &send, PW_SERVICE_TYPE, &service, 0, 0)) {
		rc_avpair_free(send);
	 	return -18;
	}

	/* Send request */
	ret = rc_auth(rh, SIP_PORT, send, &received, msg);
	rc_avpair_free(send);
	if (ret == OK_RC) {
		DBG("gsm_authorize_rad: Success ...\n");
		rc_avpair_free(received);
		return 0;
	} else {
		DBG("gsm_authorize_rad: Failure ret=[%d] err=[%d]...\n", ret, ERROR_RC);
		if(ret == BADRESP_RC)
		{ /*there could be a challenge */
		     /* Make a copy of nonce if available */
			vp = rc_avpair_get(received, PW_CHAP_CHALLENGE, 0);
			if(vp) 
			{
				if(_nonce && _nonce->s)
				{
					if (_nonce->len < vp->lvalue) {
						LOG(L_ERR, "gsm_authorize_rad: nonce buffer too small\n");
						return -20;
					}
					memcpy(_nonce->s, vp->strvalue, vp->lvalue);
					_nonce->len = vp->lvalue;
				}
				rc_avpair_free(received);
				return 1;
			}
			if(_nonce && _nonce->s)
				_nonce->len = 0;
		}
		rc_avpair_free(received);
		return -21;
	}
}

