/*
 * $Id: tm_bind.h,v 1.1 2002/05/10 01:04:45 jku Exp $
 */

#ifndef _TM_BIND_H
#define _TM_BIND_H

#include "../../sr_module.h"
#include "../tm/t_hooks.h"

struct tm_binds {
	register_tmcb_f	register_tmcb;
	cmd_function	t_isflagset;
};

extern struct tm_binds tmb; 

int bind_tm();


#endif
