/*
 * $Id: action.h,v 1.2 2002/05/13 01:15:40 jku Exp $
 *
 */

#ifndef action_h
#define action_h

#include "parser/msg_parser.h"
#include "route_struct.h"

int do_action(struct action* a, struct sip_msg* msg);
int run_actions(struct action* a, struct sip_msg* msg);





#endif
