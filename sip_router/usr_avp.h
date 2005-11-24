/*
 * $Id: usr_avp.h,v 1.14 2005/11/24 15:03:54 janakj Exp $
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
 * ---------
 *  2004-07-21  created (bogdan)
 *  2004-11-14  global aliases support added
 *  2005-02-14  list with FLAGS USAGE added (bogdan)
 */

#ifndef _SER_USR_AVP_H_
#define _SER_USR_AVP_H_

#include <sys/types.h>
#include <regex.h>


/*
 *   LIST with the allocated flags, their meaning and owner
 *   flag no.    owner            description
 *   -------------------------------------------------------
 *     0        avp_core          avp has a string name
 *     1        avp_core          avp has a string value
 *     2        avp_core          regex search in progress
 *     3        avpops module     avp was loaded from DB
 *     4        lcr module        contact avp qvalue change
 *     5        core              avp is in user list
 *     6        core              avp is in domain list
 *     7        core              avp is in global list
 *     8        core              avp is in the from avp list
 *     9        core              avp is in the to avp list
 *
 */

#include "str.h"


#define AVP_UID          "uid"           /* Unique user identifier */
#define AVP_DID          "did"           /* Unique domain identifier */
#define AVP_REALM        "digest_realm"  /* Digest realm */
#define AVP_FR_TIMER     "fr_timer"      /* Value of final response timer */
#define AVP_FR_INV_TIMER "fr_inv_timer"  /* Value of final response invite timer */
#define AVP_RPID         "rpid"          /* Remote-Party-ID */
#define AVP_GFLAGS       "gflags"        /* global flags */

struct str_int_data {
	str name;
	int val;
};

struct str_str_data {
	str name;
	str val;
};

typedef union {
	int  n;
	str  s;
	regex_t* re;
} int_str;


typedef struct usr_avp {
	unsigned short id;
	     /* Flags that are kept for the AVP lifetime */
	unsigned short flags;
	struct usr_avp *next;
	void *data;
} avp_t;

typedef avp_t* avp_list_t;


/*
 * AVP search state
 */
struct search_state {
	unsigned short flags;  /* Type of search and additional flags */
	unsigned short id;
	int_str name;
	avp_t* avp;            /* Current AVP */
	regex_t* search_re;    /* Compiled regular expression */
};

/* avp aliases structs*/
struct avp_spec {
	int type;
	int_str name;
};

/* AVP types */
#define AVP_NAME_STR     (1<<0)
#define AVP_VAL_STR      (1<<1)
#define AVP_NAME_RE      (1<<2)

/* AVP classes */
#define AVP_CLASS_USER   (1<<5)
#define AVP_CLASS_DOMAIN (1<<6)
#define AVP_CLASS_GLOBAL (1<<7)

/* AVP track (either from or to) */
#define AVP_TRACK_FROM   (1<<8)
#define AVP_TRACK_TO     (1<<9)
#define AVP_TRACK_ALL    (AVP_TRACK_FROM|AVP_TRACK_TO)

#define AVP_CLASS_ALL (AVP_CLASS_USER|AVP_CLASS_DOMAIN|AVP_CLASS_GLOBAL)

#define GALIAS_CHAR_MARKER  '$'

/* Initialize memory structures */
int init_avps(void);

/* add avp to the list of avps */
int add_avp(unsigned short flags, int_str name, int_str val);
int add_avp_list(avp_list_t* list, unsigned short flags, int_str name, int_str val);

/* Delete avps with given type and name */
int delete_avp(unsigned short flags, int_str name);

/* search functions */
avp_t *search_first_avp( unsigned short flags, int_str name,
			 int_str *val, struct search_state* state);
avp_t *search_next_avp(struct search_state* state, int_str *val);

/* free functions */
void reset_avps(void);

void destroy_avp(avp_t *avp);
void destroy_avp_list(avp_list_t *list );
void destroy_avp_list_unsafe(avp_list_t *list );

/* get func */
void get_avp_val(avp_t *avp, int_str *val );
str* get_avp_name(avp_t *avp);

avp_list_t get_avp_list(unsigned short flags);
avp_list_t set_avp_list(unsigned short flags, avp_list_t* list);


/* global alias functions (manipulation and parsing)*/
int add_avp_galias_str(char *alias_definition);
int lookup_avp_galias(str *alias, int *type, int_str *avp_name);
int add_avp_galias(str *alias, int type, int_str avp_name);
int parse_avp_name( str *name, int *type, int_str *avp_name);
int parse_avp_spec( str *name, int *type, int_str *avp_name);

#endif
