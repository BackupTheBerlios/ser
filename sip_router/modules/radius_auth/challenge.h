/*
 * $Id: challenge.h,v 1.1 2002/09/03 14:08:41 ssi Exp $
 *
 * Challenge related functions
 */

#ifndef CHALLENGE_H
#define CHALLENGE_H

#include "../../parser/msg_parser.h"


/* 
 * Challenge a user agent using WWW-Authenticate header field
 */
int radius_www_challenge(struct sip_msg* _msg, char* _realm, char* _str2);


/*
 * Challenge a user agent using Proxy-Authenticate header field
 */
int radius_proxy_challenge(struct sip_msg* _msg, char* _realm, char* _str2);


#endif /* CHALLENGE_H */
