/*
 *
 * $Id: t_stats.c,v 1.20 2005/12/21 17:25:32 janakj Exp $
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
 *  2003-06-27  tm_stats & friends freed on exit only if non-null (andrei)
 */


#include "defs.h"


#include <stdio.h>
#include "t_stats.h"
#include "../../mem/shm_mem.h"
#include "../../dprint.h"
#include "../../config.h"
#include "../../pt.h"

struct t_stats *tm_stats=0;


int init_tm_stats(void)
{
	int size;

	tm_stats=shm_malloc(sizeof(struct t_stats));
	if (tm_stats==0) {
		LOG(L_ERR, "ERROR: init_tm_stats: no mem for stats\n");
		goto error0;
	}
	memset(tm_stats, 0, sizeof(struct t_stats) );

	size=sizeof(stat_counter)*process_count;
	tm_stats->s_waiting=shm_malloc(size);
	if (tm_stats->s_waiting==0) {
		LOG(L_ERR, "ERROR: init_tm_stats: no mem for stats\n");
		goto error1;
	}
	memset(tm_stats->s_waiting, 0, size );

	tm_stats->s_transactions=shm_malloc(size);
	if (tm_stats->s_transactions==0) {
		LOG(L_ERR, "ERROR: init_tm_stats: no mem for stats\n");
		goto error2;
	}
	memset(tm_stats->s_transactions, 0, size );

	tm_stats->s_client_transactions=shm_malloc(size);
	if (tm_stats->s_client_transactions==0) {
		LOG(L_ERR, "ERROR: init_tm_stats: no mem for stats\n");
		goto error3;
	}
	memset(tm_stats->s_client_transactions, 0, size );
		 
	return 1;

error3:
	shm_free(tm_stats->s_transactions);
	tm_stats->s_transactions=0;
error2:
	shm_free(tm_stats->s_waiting);
	tm_stats->s_waiting=0;
error1:
	shm_free(tm_stats);
error0:
	return -1;
}

void free_tm_stats()
{
	if (tm_stats!=0){
		if (tm_stats->s_client_transactions) 
			shm_free(tm_stats->s_client_transactions);
		if (tm_stats->s_transactions)
			shm_free(tm_stats->s_transactions);
		if (tm_stats->s_waiting)
			shm_free(tm_stats->s_waiting);
		shm_free(tm_stats);
	}
}


const char* tm_rpc_stats_doc[2] = {
	"Print transaction statistics.",
	0
};

/* we don't worry about locking data during reads (unlike
 * setting values which always happens from some locks) 
 */
void tm_rpc_stats(rpc_t* rpc, void* c)
{
	void* st;
	unsigned long total, current, waiting, total_local;
	int i, pno;

	pno = process_count;
	for(i = 0, total = 0, waiting = 0, total_local = 0; i < pno; i++) {
		total += tm_stats->s_transactions[i];
		waiting += tm_stats->s_waiting[i];
		total_local += tm_stats->s_client_transactions[i];
	}
	current = total - tm_stats->deleted;
	waiting -= tm_stats->deleted;

	if (rpc->add(c, "{", &st) < 0) return;

	rpc->struct_add(st, "dd", "current", current, "waiting", waiting);
	rpc->struct_add(st, "dd", "waiting", waiting, "total", total);
	rpc->struct_add(st, "d", "total_local", total_local);
	rpc->struct_add(st, "d", "replied_localy", tm_stats->replied_localy);
	rpc->struct_add(st, "ddddd", 
			"6xx", tm_stats->completed_6xx,
			"5xx", tm_stats->completed_5xx,
			"4xx", tm_stats->completed_4xx,
			"3xx", tm_stats->completed_3xx,
			"2xx", tm_stats->completed_2xx);
	rpc->fault(c, 100, "Trying");
}
