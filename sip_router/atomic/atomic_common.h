/* 
 * $Id: atomic_common.h,v 1.2 2007/06/05 16:35:27 andrei Exp $
 * 
 * Copyright (C) 2006 iptelorg GmbH
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
 * common part for all the atomic operations (atomic_t and common operations)
 *  See atomic_ops.h for more info.
 */
/* 
 * History:
 * --------
 *  2006-03-08  created by andrei
 *  2007-05-13  split from atomic_ops.h (andrei)
 */
#ifndef __atomic_common
#define __atomic_common

/* atomic_t defined as a struct to easily catch non atomic ops. on it,
 * e.g.  atomic_t  foo; foo++  will generate a compile error */
typedef struct{ volatile int val; } atomic_t; 


/* store and load operations are atomic on all cpus, note however that they
 * don't include memory barriers so if you want to use atomic_{get,set} 
 * to implement mutexes you must use the mb_* versions or explicitely use
 * the barriers */

#define atomic_set_int(pvar, i) (*(int*)(pvar)=i)
#define atomic_set_long(pvar, i) (*(long*)(pvar)=i)
#define atomic_get_int(pvar) (*(int*)(pvar))
#define atomic_get_long(pvar) (*(long*)(pvar))

#define atomic_set(at_var, value)	(atomic_set_int(&((at_var)->val), (value)))

inline static int atomic_get(atomic_t *v)
{
	return atomic_get_int(&(v->val));
}


#endif
