/*
 * $Id: group.h,v 1.3 2002/05/13 01:15:40 jku Exp $
 */

#ifndef GROUP_H
#define GROUP_H

#include "../../parser/msg_parser.h"


int is_user(struct sip_msg* _msg, char* _user, char* _str2);

int is_in_group(struct sip_msg* _msg, char* _group, char* _str2);


#endif
