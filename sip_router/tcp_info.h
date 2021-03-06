/*
 * $Id: tcp_info.h,v 1.5 2009/03/05 17:20:42 andrei Exp $
 *
 * Copyright (C) 2006 iptelorg GmbH
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
/*
 * tcp various information
 */

#ifndef _tcp_info_h
#define _tcp_info_h

struct tcp_gen_info{
	int tcp_readers;
	int tcp_max_connections; /* startup connection limit, cannot be exceeded*/
	int tcp_connections_no; /* crt. number */
	int tcp_write_queued; /* total bytes queued for write, 0 if no
							 write queued support is enabled */
};




void tcp_get_info(struct tcp_gen_info* ti);

#endif
