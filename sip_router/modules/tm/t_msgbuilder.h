/*
 * $Id: t_msgbuilder.h,v 1.4 2002/09/19 12:23:55 jku Exp $
 *
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


#ifndef _MSGBUILDER_H
#define _MSGBUILDER_H

#define CSEQ "CSeq: "
#define CSEQ_LEN 6
#define TO "To: "
#define TO_LEN 4
#define CALLID "Call-ID: "
#define CALLID_LEN 9
#define CONTENT_LENGTH "Content-Length: "
#define CONTENT_LENGTH_LEN 16
#define FROM "From: "
#define FROM_LEN 6
#define FROMTAG ";tag="
#define FROMTAG_LEN 5

#define UAC_CSEQNR "1"
#define UAC_CSEQNR_LEN 1

#define UAC_CSEQNR "1"
#define UAC_CSEQNR_LEN 1

/* convenience macros */
#define memapp(_d,_s,_len) \
	do{\
		memcpy((_d),(_s),(_len));\
		(_d) += (_len);\
	}while(0);

#define  append_mem_block(_d,_s,_len) \
	do{\
		memcpy((_d),(_s),(_len));\
		(_d) += (_len);\
	}while(0);

#ifdef _OBSO
#define append_str(_p,_str) \
	do{ \
		memcpy((_p), (_str).s, (_str).len); \
		(_p)+=(_str).len); \
	} while(0);
#endif

char *build_local(struct cell *Trans, unsigned int branch,
	unsigned int *len, char *method, int method_len, str *to);

char *build_uac_request(  str msg_type, str dst, str from,
	str headers, str body, int branch,
	struct cell *t, int *len);

int t_calc_branch(struct cell *t,
	int b, char *branch, int *branch_len);
int t_setbranch( struct cell *t, struct sip_msg *msg, int b );


#endif
