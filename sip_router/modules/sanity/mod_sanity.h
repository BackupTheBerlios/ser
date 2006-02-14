/*
 * $Id: mod_sanity.h,v 1.2 2006/02/12 18:10:34 calrissian Exp $
 *
 * Sanity Checks Module
 *
 * Copyright (C) 2006 iptelorg GbmH
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
 */

#ifndef MOD_SANITY_CHK_H
#define MOD_SANITY_CHK_H

#include "../../str.h"
#include "../../parser/msg_parser.h"

#define SANITY_RURI_SIP_VERSION        (1<<0)
#define SANITY_RURI_SCHEME             (1<<1)
#define SANITY_REQUIRED_HEADERS        (1<<2)
#define SANITY_VIA_SIP_VERSION         (1<<3)
#define SANITY_VIA_PROTOCOL            (1<<4)
#define SANITY_CSEQ_METHOD             (1<<5)
#define SANITY_CSEQ_VALUE              (1<<6)
#define SANITY_CL                      (1<<7)
#define SANITY_EXPIRES_VALUE           (1<<8)
#define SANITY_PROXY_REQUIRE           (1<<9)

/* RURI_SCHEME, VIA_SIP_VERSION and VIA_PROTOCOL do not work yet */
#define SANITY_DEFAULT_CHECKS 	SANITY_RURI_SIP_VERSION | \
								SANITY_RURI_SCHEME | \
								SANITY_REQUIRED_HEADERS | \
								SANITY_CSEQ_METHOD | \
								SANITY_CSEQ_VALUE | \
								SANITY_CL | \
								SANITY_EXPIRES_VALUE | \
								SANITY_PROXY_REQUIRE

#define SANITY_MAX_CHECKS 		SANITY_RURI_SIP_VERSION | \
								SANITY_RURI_SCHEME | \
								SANITY_REQUIRED_HEADERS | \
								SANITY_VIA_SIP_VERSION | \
								SANITY_VIA_PROTOCOL | \
								SANITY_CSEQ_METHOD | \
								SANITY_CSEQ_VALUE | \
								SANITY_CL | \
								SANITY_EXPIRES_VALUE | \
								SANITY_PROXY_REQUIRE

struct _strlist {
	str string;            /* the string */
	struct _strlist* next; /* the next strlist element */
};

typedef struct _strlist strl;

extern int default_checks;
extern strl* proxyrequire_list;

/*
 * sl_send_reply function pointer
 */
int (*sl_reply)(struct sip_msg* _m, char* _s1, char* _s2);

#endif /* MOD_SANITY_CHK_H */