/* 
 * $Id: contact_parser.h,v 1.2 2002/01/07 04:39:55 jku Exp $ 
 */

#ifndef __CONTACT_PARSER_H__
#define __CONTACT_PARSER_H__

#include <time.h>
#include "location.h"

int parse_contact_field(char* _b, location_t* _loc);

#endif
