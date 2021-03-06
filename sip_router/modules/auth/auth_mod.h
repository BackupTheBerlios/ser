/*
 * $Id: auth_mod.h,v 1.26 2008/01/24 13:29:03 janakj Exp $
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
 *
 * History:
 * --------
 * 2003-04-28 rpid contributed by Juha Heinanen added (janakj)
 * 2007-10-19 auth extra checks: longer nonces that include selected message
 *            parts to protect against various reply attacks without keeping
 *            state (andrei)
 */

#ifndef AUTH_MOD_H
#define AUTH_MOD_H

#include "../../str.h"
#include "../sl/sl.h"
#include "../../parser/msg_parser.h"    /* struct sip_msg */
#include "../../parser/digest/digest.h"
#include "nonce.h" /* auth_extra_checks & AUTH_CHECK flags */

/*
 * Module parameters variables
 */
extern str secret1;            /* secret phrase used to generate nonce */
extern str secret2;            /* secret phrase used to generate nonce */
extern int nonce_expire;      /* nonce expire interval */
extern int protect_contacts;  /* Enable/disable contact hashing in nonce */
extern sl_api_t sl;
extern avp_ident_t challenge_avpid;
extern str proxy_challenge_header;
extern str www_challenge_header;
extern struct qp qop;

#endif /* AUTH_MOD_H */
