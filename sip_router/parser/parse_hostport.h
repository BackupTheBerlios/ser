/*
 * $Id: parse_hostport.h,v 1.1 2002/07/08 17:14:39 janakj Exp $
 */

#ifndef PARSE_HOSTPORT_H
#define PARSE_HOSTPORT_H

#include "../str.h"

char* parse_hostport(char* buf, str* host, short int* port);

#endif
