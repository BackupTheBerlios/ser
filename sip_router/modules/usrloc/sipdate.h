/* 
 * $Id: sipdate.h,v 1.2 2002/01/07 04:39:55 jku Exp $ 
 */

#ifndef __SIPDATE_H__
#define __SIPDATE_H__

#include <time.h>


int parse_SIP_date(char* _date, time_t* _time);


#endif
