/* 
 * $Id: endianness.c,v 1.1 2008/06/13 20:09:54 andrei Exp $
 * 
 * Copyright (C) 2008 iptelorg GmbH
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 *  endianness compile and runtime  tests
 * 
 * History:
 * --------
 *  2008-06-13  created by andrei
 */

#include "endianness.h"

int _endian_test_int=1 /* used for the runtime endian tests */;


/* return 0 on success, -1 on error (compile time detected endianness is
 * different from run time)
 */
int endianness_sanity_check()
{
#ifdef __IS_LITTLE_ENDIAN
	return is_little_endian()-1;
#elif defined __IS_BIG_ENDIAN
	return is_big_endian()-1;
#else
#warning BUG: endianness macro are not defined
	return -1;
#endif
}

