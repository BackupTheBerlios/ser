/*
 * $Id: auth.h,v 1.7 2002/03/08 01:09:35 janakj Exp $
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


/*
 * Test for user id
 */
int is_user(struct sip_msg* _msg, char* _user, char* _str2);


/*
 * Test if the user belongs to given group
 */
int is_in_group(struct sip_msg* _msg, char* _group, char* _str2);


/*
 * Compare auth id and username in To HF
 */
int check_to(struct sip_msg* _msg, char* _str1, char* _str2);


/*
 * Compare auth id and username in From HF
 */
int check_from(struct sip_msg* _msg, char* _str1, char* _str2);


#endif
