/*
 * $Id: parse_from.h,v 1.2 2002/08/08 17:32:59 janakj Exp $
 */

#ifndef PARSE_FROM
#define PARSE_FROM

#include "../str.h"
#include "parse_to.h"
/*
 * To header field parser
 */
int parse_from_header(struct hdr_field* hdr);

#endif
