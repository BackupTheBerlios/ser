/*
 * $Id: auth.h,v 1.5 2002/02/26 17:36:26 janakj Exp $
 */

#ifndef AUTH_H
#define AUTH_H

#include "../../msg_parser.h"


/*
 * Initialize authentication module
 */
void auth_init(void);


/*
 * Challenge a user agent, the first parameter is realm
 */
int challenge(struct sip_msg* _msg, char* _realm, char* _str2);


/*
 * Try to autorize request from a user, the first parameter
 * is realm
 */
int authorize(struct sip_msg* _msg, char* _realm, char* _str2);


#endif
