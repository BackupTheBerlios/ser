/*
 * $Id: reply.h,v 1.1 2002/08/21 20:00:56 janakj Exp $
 *
 * Send a reply
 */

#ifndef REPLY_H
#define REPLY_H

#include "../../parser/msg_parser.h"
#include "../usrloc/usrloc.h"


/*
 * Send a reply
 */
int send_reply(struct sip_msg* _m);


/*
 * Build Contact HF for reply
 */
void build_contact(ucontact_t* _c);


#endif /* REPLY_H */
