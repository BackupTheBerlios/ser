/*
 * $Id: hash_func.h,v 1.8 2006/12/13 22:50:46 andrei Exp $
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



#ifndef _HASH_H
#define _HASH_H

#include "str.h"
#include "hashes.h"

/* always use a power of 2 for hash table size */
#define T_TABLE_POWER    16 
#define TABLE_ENTRIES    (1 << (T_TABLE_POWER))

unsigned int new_hash( str  call_id, str cseq_nr );

#define new_hash2(call_id, cseq_nr) \
	(get_hash2_raw(&(call_id), &(cseq_nr)) & (TABLE_ENTRIES-1))


#define hash( cid, cseq) new_hash2( cid, cseq )

#endif
