/*
 * $Id: nathelper.h,v 1.7 2008/11/04 21:10:07 sobomax Exp $
 *
 *
 * Copyright (C) 2005 Porta Software Ltd.
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

#ifndef _NATHELPER_H
#define _NATHELPER_H

/* Handy macros */
#define	STR2IOVEC(sx, ix)	do {(ix).iov_base = (sx).s; (ix).iov_len = (sx).len;} while(0)
#define SZ2IOVEC(sx, ix)	do {char *_t_p = (ix).iov_base = (sx); (ix).iov_len = strlen(_t_p);} while(0)

/* Parameters from nathelper.c */
extern struct socket_info* force_socket;

/* Functions from nathelper */
struct rtpp_node *select_rtpp_node(str, int, int);
char *send_rtpp_command(struct rtpp_node *, struct iovec *, int);

/* Functions from natping.c */
int natpinger_init(void);
int natpinger_child_init(int);
int natpinger_cleanup(void);

int natping_contact(str, struct dest_info *);

int intercept_ping_reply(struct sip_msg* msg);

/* Variables from natping.c referenced from nathelper.c */
extern int natping_interval;
extern int ping_nated_only;
extern char *natping_method;
extern int natping_stateful;
extern int natping_crlf;

#endif
