/*
 * $Id: tm_bind.c,v 1.1 2002/05/10 01:04:45 jku Exp $
 */

#include "tm_bind.h"

struct tm_binds tmb;


int bind_tm()
{
	if (!( tmb.t_isflagset=find_export("t_isflagset", 1)) ) {
		LOG(L_ERR, "ERROR: mod_init(acc): TM module function 't_isflagset' not found\n");
		return -1;
	}
	if (!( tmb.register_tmcb=(register_tmcb_f) find_export("register_tmcb", 2)) ) {
		LOG(L_ERR, "ERROR: mod_init(acc): TM module function 'register_tmcb' not found\n");
		return -1;
	}
}
