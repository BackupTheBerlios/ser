/*
 * $Id: utime.c,v 1.3 2002/08/21 20:09:02 janakj Exp $
 *
 * Usrloc time related functions
 */

#include "utime.h"


time_t act_time;


void get_act_time(void)
{

	act_time = time(0);
}
