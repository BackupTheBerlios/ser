/*
 * $Id: common.h,v 1.1 2002/08/09 11:17:14 janakj Exp $
 *
 * Common function needed by authorize
 * and challenge related functions
 */

#ifndef COMMON_H
#define COMMON_H

#include "../../parser/msg_parser.h"


/*
 * Send a response
 */
int send_resp(struct sip_msg* _m, int _code, char* _reason, char* _hdr, int _hdr_len);


#endif /* COMMON_H */
