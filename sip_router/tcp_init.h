/*
 * $Id: tcp_init.h,v 1.5 2007/07/25 19:40:32 andrei Exp $
 *
 * Copyright (C) 2001-2003 FhG Fokus
 *
 * This file is part of ser, a free SIP server.
 *
 * ser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * For a license to use the ser software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact iptel.org by e-mail at the following addresses:
 *    info@iptel.org
 *
 * ser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef tcp_init_h
#define tcp_init_h
#include "ip_addr.h"

struct tcp_child{
	pid_t pid;
	int proc_no; /* ser proc_no, for debugging */
	int unix_sock; /* unix "read child" sock fd */
	int busy;
	int n_reqs; /* number of requests serviced so far */
};


int init_tcp();
void destroy_tcp();
int tcp_init(struct socket_info* sock_info);
int tcp_init_children();
void tcp_main_loop();
void tcp_receive_loop(int unix_sock);
int tcp_fix_child_sockets(int* fd);

/* sets source address used when opening new sockets and no source is specified
 *  (by default the address is choosen by the kernel)
 * Should be used only on init.
 * returns -1 on error */
int tcp_set_src_addr(struct ip_addr* ip);

#endif
