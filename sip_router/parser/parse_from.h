/*
 * $Id: parse_from.h,v 1.4 2002/08/08 20:28:03 janakj Exp $
 */

#ifndef PARSE_FROM
#define PARSE_FROM

#include "../str.h"
#include "parse_to.h"
#include "hf.h"
/*
 * To header field parser
 */
int parse_from_header(struct hdr_field* hdr);

#endif
