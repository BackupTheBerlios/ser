/*
 * $Id: udp_server.h,v 1.2 2001/09/06 02:24:00 andrei Exp $
 */

#ifndef udp_server_h
#define udp_server_h



extern int udp_sock;

int udp_init(unsigned long ip, unsigned short port);
int udp_rcv_loop();


#endif
