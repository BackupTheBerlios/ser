/*
 * $Id: api.h,v 1.7 2005/02/23 17:16:02 andrei Exp $
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

#ifndef API_H
#define API_H


#include "../../parser/msg_parser.h"
#include "../../parser/hf.h"
#include "../../str.h"


typedef enum auth_result {
	ERROR = -2 ,        /* Error occurred, a reply has been sent out -> return 0 to the ser core */
	NOT_AUTHORIZED,     /* Don't perform authorization, credentials missing */
	DO_AUTHORIZATION,   /* Perform digest authorization */
        AUTHORIZED          /* Authorized by default, no digest authorization necessary */
} auth_result_t;


/*
 * Purpose of this function is to find credentials with given realm,
 * do sanity check, validate credential correctness and determine if
 * we should really authenticate (there must be no authentication for
 * ACK and CANCEL
 */
typedef auth_result_t (*pre_auth_t)(struct sip_msg* _m, str* _realm,
								hdr_types_t _hftype, struct hdr_field** _h);
auth_result_t pre_auth(struct sip_msg* _m, str* _realm, hdr_types_t _hftype,
						struct hdr_field** _h);


/*
 * Purpose of this function is to do post authentication steps like
 * marking authorized credentials and so on.
 */
typedef auth_result_t (*post_auth_t)(struct sip_msg* _m, struct hdr_field* _h);
auth_result_t post_auth(struct sip_msg* _m, struct hdr_field* _h);

/*
 * Strip the beginning of realm
 */
void strip_realm(str *_realm);


/*
 * Auth module API
 */
typedef struct auth_api {
	str rpid_avp;           /* Name of AVP containing Remote-Party-ID */
	pre_auth_t pre_auth;    /* The function to be called before authentication */
	post_auth_t post_auth;  /* The function to be called after authentication */
} auth_api_t;

typedef int (*bind_auth_t)(auth_api_t* api);
int bind_auth(auth_api_t* api);


#endif /* API_H */
