/*
 * $Id: regtime.c,v 1.1 2002/08/21 20:18:12 janakj Exp $
 *
 * Registrar time related functions
 */

#include "regtime.h"


time_t act_time;


/*
 * Get actual time and store
 * value in act_time
 */
void get_act_time(void)
{
	act_time = time(0);
}
