/*
 * $Id: group.h,v 1.2 2002/05/11 21:27:30 jku Exp $
 */

#ifndef GROUP_H
#define GROUP_H

#include "../../msg_parser.h"


int is_user(struct sip_msg* _msg, char* _user, char* _str2);

int is_in_group(struct sip_msg* _msg, char* _group, char* _str2);


#endif
