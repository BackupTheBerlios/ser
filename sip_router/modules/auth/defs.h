/* 
 * $Id: defs.h,v 1.9 2002/05/13 20:09:06 jku Exp $ 
 */
#ifndef __DEFS_H__
#define __DEFS_H__

#define PARANOID

#define WWW_AUTH_RESPONSE "Authorization"
#define PROXY_AUTH_RESPONSE  "Proxy-Authorization"

#define WWW_AUTH_CHALLENGE "WWW-Authenticate"
#define PROXY_AUTH_CHALLENGE "Proxy-Authenticate"

#define AUTH_HF_LEN 512

/*
 * Helper definitions
 */

#define MESSAGE_407 "Proxy Authentication Required"
#define MESSAGE_401 "Unauthorized"
#define MESSAGE_400 "Bad Request"
#define MESSAGE_403 "Forbidden"

#define USER_DOMAIN_HACK
#define ACK_CANCEL_HACK

/* print algorithm name in challenge explicitely */
#define PRINT_MD5

#endif
