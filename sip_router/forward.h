/*
 *  $Id: forward.h,v 1.2 2001/09/06 02:24:00 andrei Exp $
 */


#ifndef forward_h
#define forward_h

#include "msg_parser.h"
#include "route.h"


int check_address(unsigned long ip, char *name, int resolver);

int forward_request(char * orig, char* buf, unsigned int len,
					 struct sip_msg* msg,  struct route_elem* re,
					 unsigned long source_ip);

int forward_reply(char * orig, char* buf, unsigned int len, 
					struct sip_msg* msg);

#endif
