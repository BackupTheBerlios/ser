/*
 * $Id: checks.h,v 1.3 2002/05/13 01:15:40 jku Exp $
 */

#ifndef CHECKS_H
#define CHECKS_H

#include "../../parser/msg_parser.h"
#include "../../str.h"


int check_to(struct sip_msg* _msg, char* _str1, char* _str2);
int check_from(struct sip_msg* _msg, char* _str1, char* _str2);

#endif
