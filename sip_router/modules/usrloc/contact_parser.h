/* 
 * $Id: contact_parser.h,v 1.4 2002/04/12 00:36:01 janakj Exp $ 
 */

#ifndef CONTACT_PARSER_H
#define CONTACT_PARSER_H

#include <time.h>
#include "location.h"

int parse_contact_hdr(char* _b, location_t* _loc, int _expires, int* _star,
		      const char* _callid, int _cseq);

#endif
