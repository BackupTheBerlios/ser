/*
 * $Id: checks.h,v 1.7 2002/12/08 17:18:56 janakj Exp $
 *
 * Check if to and from contain the same username as
 * in digest credentials
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


#ifndef CHECKS_H
#define CHECKS_H

#include "../../parser/msg_parser.h"


/*
 * Check if given username matches those in digest credentials
 */
int is_user(struct sip_msg* _msg, char* _user, char* _str2);


/*
 * Check if To header field contains the same username
 * as digest credentials
 */
int check_to(struct sip_msg* _msg, char* _str1, char* _str2);


/*
 * Check if From header field contains the same username
 * as digest credentials
 */
int check_from(struct sip_msg* _msg, char* _str1, char* _str2);


/*
 * Check if host in From uri is local
 */
int is_from_local(struct sip_msg* _msg, char* _table, char* _s2);


/*
 * Check if host in Request URI is local
 */
int is_uri_host_local(struct sip_msg* _msg, char* _table, char* _s2);


/*
 * Check if uri belongs to a local user
 */
int does_uri_exist(struct sip_msg* _msg, char* _table, char* _s2);


#endif /* CHECKS_H */
