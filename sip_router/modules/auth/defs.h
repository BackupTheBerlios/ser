/* 
 * $Id: defs.h,v 1.7 2002/04/22 12:57:23 janakj Exp $ 
 */

#ifndef __DEFS_H__
#define __DEFS_H__

#define PARANOID

#define AUTH_RESPONSE  "Proxy-Authorization"

#define AUTH_HF_LEN 512


#define NONCE_SECRET "4e9rhygt90ofw34e8hiof09tg"
#define NONCE_SECRET_LEN 25

#define GRP_TABLE "grp"
#define GRP_USER "user"
#define GRP_GRP "grp"


/*
 * Helper definitions
 */

#define MESSAGE_407 "Proxy Authentication Required"
#define MESSAGE_400 "Bad Request"

#endif
