/*
 * $Id: parse_hname.h,v 1.1 2002/07/08 17:14:39 janakj Exp $
 */

#ifndef PARSE_HNAME_H
#define PARSE_HNAME_H

#include "hf.h"

/* returns end or pointer to next elem*/
char* parse_hname1(char* p, char* end, struct hdr_field* hdr);

#endif
