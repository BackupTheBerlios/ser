/*
 * $Id: utime.c,v 1.1 2002/08/21 20:02:12 janakj Exp $
 *
 * Usrloc time related functions
 */

#include "utime.h"


time_t act_time;


void get_act_time(void)
{

	act_time = time(0);
}
