/* 
 * $Id: case_dive.h,v 1.3 2005/02/23 17:16:07 andrei Exp $ 
 *
 * Diversion Header Field Parsing Macros
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


#ifndef CASE_DIVE_H
#define CASE_DIVE_H


#define RSIO_CASE                                  \
        switch(LOWER_DWORD(val)) {                 \
        case _rsio_:                               \
	        p += 4;                            \
	        if (LOWER_BYTE(*p) == 'n') {       \
		        hdr->type = HDR_DIVERSION_T; \
                        p++;                       \
                        goto dc_end;               \
                }                                  \
                goto other;                        \
        }


#define dive_CASE         \
        p += 4;           \
        val = READ(p);    \
        RSIO_CASE;        \
        goto other;


#endif /* CASE_DIVE_H */
