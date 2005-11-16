/* 
 * Copyright (C) 2005 iptelorg GmbH
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

#ifndef __PRESENCE_INFO_H
#define __PRESENCE_INFO_H

#include <cds/sstr.h>
#include <time.h>

typedef enum {
	presence_tuple_open,
	presence_tuple_closed
} presence_tuple_status_t;

typedef enum {
	presence_auth_rejected,
	presence_auth_polite_block,
	presence_auth_unresolved,
	presence_auth_granted
} presence_authorization_status_t;

typedef struct _presence_typle_info_t {
	str_t contact;
	double priority;
	time_t expires;
	presence_tuple_status_t status;
	str_t extra_status;
	struct _presence_typle_info_t *next, *prev;
} presence_tuple_info_t;

typedef struct {
	str_t presentity; /* do not modify this !*/
	presence_tuple_info_t *first_tuple, *last_tuple;
	presence_authorization_status_t auth;
	char presentity_data[1];
} presentity_info_t;

presentity_info_t *create_presentity_info(const str_t *presentity);
presence_tuple_info_t *create_tuple_info(const str_t *contact, presence_tuple_status_t status);
void add_tuple_info(presentity_info_t *p, presence_tuple_info_t *t);
void free_presentity_info(presentity_info_t *p);


#endif