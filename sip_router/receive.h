/*
 * $Id: receive.h,v 1.3 2002/05/26 13:50:48 andrei Exp $
 */


#ifndef receive_h
#define receive_h

#include "ip_addr.h"

int receive_msg(char* buf, unsigned int len, union sockaddr_union *src_su);


#endif
