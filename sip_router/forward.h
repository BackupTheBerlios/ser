/*
 *  $Id: forward.h,v 1.9 2002/05/26 21:38:02 andrei Exp $
 */


#ifndef forward_h
#define forward_h

#include "parser/msg_parser.h"
#include "route.h"
#include "proxy.h"
#include "ip_addr.h"


struct socket_info* get_send_socket(union sockaddr_union* su);
int forward_request( struct sip_msg* msg,  struct proxy_l* p);
int update_sock_struct_from_via( union sockaddr_union* to,
								struct via_body* via );
int forward_reply( struct sip_msg* msg);

#endif
