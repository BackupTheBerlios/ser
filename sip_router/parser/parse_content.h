/*
 * $Id: parse_content.h,v 1.1 2002/11/28 16:14:53 bogdan Exp $
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

#ifndef _PARSE_CONTENT_H
#define _PARSE_CONTENT_H

#define CONTENT_TYPE_UNKNOWN         0
#define CONTENT_TYPE_TEXT_PLAIN      1
#define CONTENT_TYPE_MESSAGE_CPIM    2
#define CONTENT_TYPE_APPLICATION_SDP 3


char* parse_content_type( char* buffer, char* end, int* type);
char* parse_content_length( char* buffer, char* end, int* len);

#endif
