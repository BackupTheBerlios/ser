/*
 * $Id: save.h,v 1.1 2002/08/21 20:18:12 janakj Exp $
 *
 * Functions that process REGISTER message 
 * and store data in usrloc
 */

#ifndef SAVE_H
#define SAVE_H


#include "../../parser/msg_parser.h"


/*
 * Process REGISTER request and save it's contacts
 */
int save(struct sip_msg* _m, char* _t, char* _s);


#endif /* SAVE_H */
