/*
 * $Id: gsm_avp.h,v 1.1 2003/12/09 12:43:22 dcm Exp $
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

#ifndef _GSM_AVP_H_
#define _GSM_AVP_H_

#include "../../str.h"
#include "../../parser/digest/digest_parser.h"


/*
 * This function creates and submits radius authentication request.
 * Service type of the request is Authenticate-Only.
 */
int gsm_authorize_rad(dig_cred_t* _cred, str* _user, str* _nonce); 

#endif /* _GSM_AVP_H_ */

