/*
 * $Id: parse_hname2.h,v 1.1 2002/07/08 17:14:39 janakj Exp $
 */

#ifndef PARSE_HNAME2_H
#define PARSE_HNAME2_H

#include "hf.h"


/*
 * Yet another parse_hname - Ultra Fast version :-)
 */
char* parse_hname2(char* begin, char* end, struct hdr_field* hdr);


/*
 * Initialize hash table
 */
void init_htable(void);


#endif
