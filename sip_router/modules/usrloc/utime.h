/*
 * $Id: utime.h,v 1.3 2002/08/21 20:09:02 janakj Exp $
 *
 * Usrloc time related functions
 */

#ifndef UTIME_H
#define UTIME_H

#include <time.h>


extern time_t act_time;


/*
 * Get actual time
 */
void get_act_time(void);


#endif /* UTIME_H */
