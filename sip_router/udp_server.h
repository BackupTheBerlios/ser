/*
 * $Id: udp_server.h,v 1.4 2001/09/21 20:35:51 andrei Exp $
 */

#ifndef udp_server_h
#define udp_server_h

#include <sys/types.h>
#include <sys/socket.h>

extern int udp_sock;

int udp_init(unsigned long ip, unsigned short port);
int udp_send(char *buf, unsigned len, struct sockaddr*  to, unsigned tolen);
int udp_rcv_loop();


#endif
