/*
 * $Id: parse_from.h,v 1.1 2002/08/08 17:15:41 ssi Exp $
 */

#ifndef PARSE_FROM
#define PARSE_FROM

#include "../str.h"
#include "parse_to.h"
/*
 * To header field parser
 */
char* parse_from(char* buffer, char *end, struct from_body *from_b);

#endif
