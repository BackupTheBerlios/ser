/* 
 * $Id: auth.h,v 1.2 2002/01/07 04:39:51 jku Exp $ 
 */



#ifndef __AUTH_H__
#define __AUTH_H__

#include "../../str.h"
#include "../../msg_parser.h"


#define BASIC_SCHEME   1
#define DIGEST_SCHEME  2
#define UNKNOWN_SCHEME 4

struct auth {
	int scheme;
	str username;
	str password;
} auth_t;

#endif
