/*
 * $Id: lookup.h,v 1.1 2002/08/21 20:00:56 janakj Exp $
 *
 * Lookup contacts in usrloc
 */

#ifndef LOOKUP_H
#define LOOKUP_H

#include "../../parser/msg_parser.h"


int lookup(struct sip_msg* _m, char* _t, char* _s);


#endif /* LOOKUP_H */
