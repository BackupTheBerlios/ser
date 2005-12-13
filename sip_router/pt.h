/*
 * $Id: pt.h,v 1.13 2005/12/13 12:10:16 janakj Exp $
 *
 * Process Table
 *
 *
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
/*
 * History:
 * --------
 *  2003-04-15  added tcp_disable support (andrei)
 */


#ifndef _PT_H
#define _PT_H

#include <sys/types.h>
#include <unistd.h>

#include "globals.h"
#include "timer.h"
#include "socket_info.h"

#define MAX_PT_DESC	128

struct process_table {
	int pid;
#ifdef USE_TCP
	int unix_sock; /* unix socket on which tcp main listens */
	int idx; /* tcp child index, -1 for other processes */
#endif
	char desc[MAX_PT_DESC];
};

extern struct process_table *pt;
extern int process_no;
extern int process_count;
extern int last_process;


/* return processes pid */
inline static int my_pid()
{
	return pt ? pt[process_no].pid : getpid();
}


#endif
