/*
 * $Id: common.h,v 1.3 2002/09/19 12:23:54 jku Rel $
 *
 * Common stuff
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

#include "../../str.h"


/*
 * Find a character occurence that is not quoted
 */
char* ul_fnq(str* _s, char _c);


/*
 * Extract username part from URI
 */
int ul_get_user(str* _s);


/*
 * Copy str structure, doesn't copy
 * the whole string !
 */
static inline void str_copy(str* _d, str* _s)
{
	_d->s = _s->s;
	_d->len = _s->len;
}


#endif /* COMMON_H */
