/* 
 * $Id: timer_dirty.h,v 1.2 2002/01/07 04:39:56 jku Exp $ 
 */

#ifndef __TIMER_DIRTY__
#define __TIMER_DIRTY__

#include "cache.h"

int  init_timer_dirty  (const char* _table);
void timer_dirty       (cache_t* _c);
void close_timer_dirty (void);

#endif
