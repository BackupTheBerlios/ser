/*
 * $Id: challenge.h,v 1.6 2006/01/27 13:09:00 janakj Exp $
 *
 * Challenge related functions
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


#ifndef CHALLENGE_H
#define CHALLENGE_H

#include "../../parser/msg_parser.h"


/* 
 * Challenge a user agent using WWW-Authenticate header field
 */
int www_challenge(struct sip_msg* msg, char* realm, char* str2);


/*
 * Challenge a user agent using Proxy-Authenticate header field
 */
int proxy_challenge(struct sip_msg* msg, char* realm, char* str2);


/* 
 * Challenge a user agent using WWW-Authenticate header field
 */
int www_challenge1(struct sip_msg* msg, char* qop, char* s2);


/*
 * Challenge a user agent using Proxy-Authenticate header field
 */
int proxy_challenge1(struct sip_msg* msg, char* qop, char* s2);


/*
 * Remove used credentials from a SIP message header
 */
int consume_credentials(struct sip_msg* msg, char* s1, char* s2);


#endif /* AUTH_H */
