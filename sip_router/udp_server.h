/*
 * $Id: udp_server.h,v 1.5 2001/11/12 19:51:59 jku Exp $
 */

#ifndef udp_server_h
#define udp_server_h

#include <sys/types.h>
#include <sys/socket.h>

#define MAX_RECV_BUFFER_SIZE	256*1024
#define BUFFER_INCREMENT	2048

extern int udp_sock;

int udp_init(unsigned long ip, unsigned short port);
int udp_send(char *buf, unsigned len, struct sockaddr*  to, unsigned tolen);
int udp_rcv_loop();


#endif
