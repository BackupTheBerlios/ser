/*
 * $Id: parse_hostport.h,v 1.2 2002/08/12 12:17:12 janakj Exp $
 */

#ifndef PARSE_HOSTPORT_H
#define PARSE_HOSTPORT_H

#include "../str.h"

char* parse_hostport(char* buf, str* host, short int* port);

#endif /* PARSE_HOSTPORT_H */
