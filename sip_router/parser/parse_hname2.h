/*
 * $Id: parse_hname2.h,v 1.3 2002/07/30 16:31:33 janakj Exp $
 */

#ifndef PARSE_HNAME2_H
#define PARSE_HNAME2_H

#include "hf.h"


/*
 * Fast 32-bit header field name parser
 */
char* parse_hname2(char* begin, char* end, struct hdr_field* hdr);


/*
 * Initialize hash table
 */
void init_hfname_parser(void);


#endif
