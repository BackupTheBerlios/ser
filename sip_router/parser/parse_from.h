/*
 * $Id: parse_from.h,v 1.3 2002/08/08 17:41:56 ssi Exp $
 */

#ifndef PARSE_FROM
#define PARSE_FROM

#include "../str.h"
#include "parse_to.h"
/*
 * To header field parser
 */
char* parse_from_header(char* buffer, char *end, struct to_body *from_b);

#endif
