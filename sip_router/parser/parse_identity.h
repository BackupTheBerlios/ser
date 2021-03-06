/*
 * $Id: parse_identity.h,v 1.2 2007/10/15 14:21:02 gkovacs Exp $
 *
 * Copyright (c) 2007 iptelorg GmbH
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


#ifndef PARSE_IDENTITY
#define PARSE_IDENTITY

#include "../str.h"
#include "msg_parser.h"

struct identity_body{
	int error;  		/* Error code */
	int ballocated;  	/* Does hash point to an allocated area */
	str hash;
};


/* casting macro for accessing IDENTITY body */
#define get_identity(p_msg) ((struct identity_body*)(p_msg)->identity->parsed)


/*
 * Parse Identity header field
 */
void parse_identity(char *buf, char *end, struct identity_body *ib);
int parse_identity_header(struct sip_msg *msg);


/*
 * Free all associated memory
 */
void free_identity(struct identity_body *ib);


#endif
