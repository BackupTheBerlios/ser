/*
 *  $Id: forward.h,v 1.1 2001/09/04 01:41:39 andrei Exp $
 */


#ifndef forward_h
#define forward_h

#include "msg_parser.h"
#include "route.h"

int forward_request(char * orig, char* buf, unsigned int len,
					 struct sip_msg* msg,  struct route_elem* re);

int forward_reply(char * orig, char* buf, unsigned int len, 
					struct sip_msg* msg);

#endif
