/*
 * $Id: action.h,v 1.1 2001/09/21 15:24:24 andrei Exp $
 *
 */

#ifndef action_h
#define action_h

#include "msg_parser.h"
#include "route_struct.h"

int do_action(struct action* a, struct sip_msg* msg);
int run_actions(struct action* a, struct sip_msg* msg);





#endif
