/*
 * $Id: common.h,v 1.3 2003/03/04 14:53:53 janakj Exp $
 *
 * Common function needed by authorize
 * and challenge related functions
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


#ifndef COMMON_H
#define COMMON_H

#include "../../parser/msg_parser.h"
#include "../../str.h"


/*
 * Send a response
 */
int send_resp(struct sip_msg* _m, int _code, char* _reason, char* _hdr, int _hdr_len);

char* auth_fnq(str* _b, char _c);

/*
 * Cut username part of a URL
 */
int auth_get_username(str* _s);

/*
 * Check that To header is properly parsed and if so,
 * return pointer to parsed To header.  Otherwise return NULL.
 */
struct to_body *get_parsed_to_body(struct sip_msg *_msg);

/*
 * Check that From header is properly parsed and if so,
 * return pointer to parsed From header.  Otherwise return NULL.
 */
struct to_body *get_parsed_from_body(struct sip_msg *_msg);


#endif /* COMMON_H */
