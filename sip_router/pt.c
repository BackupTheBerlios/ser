/*
 * $Id: pt.c,v 1.13 2008/08/08 20:47:53 andrei Exp $
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
 *  2006-06-14	added process table in shared mem (dragos)
 *  2006-09-20	added profile support (-DPROFILING) (hscholz)
 *  2006-10-25	sanity check before allowing forking w/ tcp support (is_main
 *               & tcp not started yet); set is_main=0 in childs (andrei)
 *  2007-07-04	added register_fds() and get_max_open_fds(() (andrei)
 */


#include "pt.h"
#include "tcp_init.h"
#include "sr_module.h"
#include "socket_info.h"
#include "rand/fastrand.h"
#ifdef PKG_MALLOC
#include "mem/mem.h"
#endif
#ifdef SHM_MEM
#include "mem/shm_mem.h"
#endif
#if defined PKG_MALLOC || defined SHM_MEM
#include "cfg_core.h"
#endif

#include <stdio.h>
#include <time.h> /* time(), used to initialize random numbers */

#define FORK_DONT_WAIT  /* child doesn't wait for parent before starting 
						   => faster startup, but the child should not assume
						   the parent fixed the pt[] entry for it */


#ifdef PROFILING
#include <sys/gmon.h>

	extern void _start(void);
	extern void etext(void);
#endif


static int estimated_proc_no=0;
static int estimated_fds_no=0;


/* number of known "common" used fds */
static int calc_common_open_fds_no()
{
	int max_fds_no;
	
	/* 1 tcp send unix socket/all_proc, 
	 *  + 1 udp sock/udp proc + 1 possible dns comm. socket + 
	 *  + 1 temporary tcp send sock.
	 */
	max_fds_no=estimated_proc_no*4 /* udp|sctp + tcp unix sock +
									  tmp. tcp send +
									  tmp dns.*/
				-1 /* timer (no udp)*/ + 3 /* stdin/out/err */;
	return max_fds_no;
}



/* returns 0 on success, -1 on error */
int init_pt(int proc_no)
{
#ifdef USE_TCP
	int r;
#endif
	
	estimated_proc_no+=proc_no;
	estimated_fds_no+=calc_common_open_fds_no();
	/*alloc pids*/
#ifdef SHM_MEM
	pt=shm_malloc(sizeof(struct process_table)*estimated_proc_no);
	process_count = shm_malloc(sizeof(int));
#else
	pt=pkg_malloc(sizeof(struct process_table)*estimated_proc_no);
	process_count = pkg_malloc(sizeof(int));
#endif
	process_lock = lock_alloc();
	process_lock = lock_init(process_lock);
	if (pt==0||process_count==0||process_lock==0){
		LOG(L_ERR, "ERROR: out  of memory\n");
		return -1;
	}
	memset(pt, 0, sizeof(struct process_table)*estimated_proc_no);
#ifdef USE_TCP
	for (r=0; r<estimated_proc_no; r++){
		pt[r].unix_sock=-1;
		pt[r].idx=-1;
	}
#endif
	process_no=0; /*main process number*/
	pt[process_no].pid=getpid();
	memcpy(pt[process_no].desc,"main",5);
	*process_count=1;
	return 0;
}


/* register no processes, used from mod_init when processes will be forked
 *  from mod_child 
 *  returns 0 on success, -1 on error
 */
int register_procs(int no)
{
	if (pt){
		LOG(L_CRIT, "BUG: register_procs(%d) called at runtime\n", no);
		return -1;
	}
	estimated_proc_no+=no;
	return 0;
}



/* returns the maximum number of processes */
int get_max_procs()
{
	if (pt==0){
		LOG(L_CRIT, "BUG: get_max_procs() called too early "
				"(it must _not_ be called from mod_init())\n");
		abort(); /* crash to quickly catch offenders */
	}
	return estimated_proc_no;
}


/* register no fds, used from mod_init when modules will open more global
 *  fds (from mod_init or child_init(PROC_INIT)
 *  or from child_init(rank) when the module will open fds local to the
 *   process "rank".
 *   (this is needed because some other parts of ser code rely on knowing
 *    the maximum open fd number in a process)
 *  returns 0 on success, -1 on error
 */
int register_fds(int no)
{
	/* can be called at runtime, but should be called from child_init() */
	estimated_fds_no+=no;
	return 0;
}



/* returns the maximum open fd number */
int get_max_open_fds()
{
	if (pt==0){
		LOG(L_CRIT, "BUG: get_max_open_fds() called too early "
				"(it must _not_ be called from mod_init())\n");
		abort(); /* crash to quickly catch offenders */
	}
	return estimated_fds_no;
}


/* return processes pid */
int my_pid()
{
	return pt ? pt[process_no].pid : getpid();
}



/* close unneeded sockets */
int close_extra_socks(int child_id, int proc_no)
{
#ifdef USE_TCP
	int r;
	struct socket_info* si;
	
	if (child_id!=PROC_TCP_MAIN){
		for (r=0; r<proc_no; r++){
			if (pt[r].unix_sock>=0){
				/* we can't change the value in pt[] because it's
				 * shared so we only close it */
				close(pt[r].unix_sock);
			}
		}
		/* close all listen sockets (needed only in tcp_main */
		if (!tcp_disable){
			for(si=tcp_listen; si; si=si->next){
				close(si->socket);
				/* safe to change since this is a per process copy */
				si->socket=-1;
			}
#ifdef USE_TLS
			if (!tls_disable){
				for(si=tls_listen; si; si=si->next){
					close(si->socket);
					/* safe to change since this is a per process copy */
					si->socket=-1;
				}
			}
#endif /* USE_TLS */
		}
		/* we still need the udp sockets (for sending) so we don't close them
		 * too */
	}
#endif /* USE_TCP */
	return 0;
}



/**
 * Forks a new process.
 * @param child_id - rank, if equal to PROC_NOCHLDINIT init_child will not be
 *                   called for the new forked process (see sr_module.h)
 * @param desc - text description for the process table
 * @param make_sock - if to create a unix socket pair for it
 * @returns the pid of the new process
 */
int fork_process(int child_id, char *desc, int make_sock)
{
	int pid, child_process_no;
	int ret;
	unsigned int new_seed1;
	unsigned int new_seed2;
#ifdef USE_TCP
	int sockfd[2];
#endif

	ret=-1;
	#ifdef USE_TCP
		sockfd[0]=sockfd[1]=-1;
		if(make_sock && !tcp_disable){
			 if (!is_main){
				 LOG(L_CRIT, "BUG: fork_process(..., 1) called from a non "
						 "\"main\" process! If forking from a module's "
						 "child_init() fork only if rank==PROC_MAIN or"
						 " give up tcp send support (use 0 for make_sock)\n");
				 goto error;
			 }
			 if (tcp_main_pid){
				 LOG(L_CRIT, "BUG: fork_process(..., 1) called, but tcp main "
						 " is already started\n");
				 goto error;
			 }
			 if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd)<0){
				LOG(L_ERR, "ERROR: fork_process(): socketpair failed: %s\n",
							strerror(errno));
				goto error;
			}
		}
	#endif
	lock_get(process_lock);
	if (*process_count>=estimated_proc_no) {
		LOG(L_CRIT, "ERROR: fork_process(): Process limit of %d exceeded."
					" Will simulate fork fail.\n", estimated_proc_no);
		lock_release(process_lock);
		goto error;
	}	
	
	
	child_process_no = *process_count;
	new_seed1=rand();
	new_seed2=random();
	pid = fork();
	if (pid<0) {
		lock_release(process_lock);
		ret=pid;
		goto error;
	}else if (pid==0){
		/* child */
		is_main=0; /* a forked process cannot be the "main" one */
		process_no=child_process_no;
		/* close tcp unix sockets if this is not tcp main */
#ifdef USE_TCP
		close_extra_socks(child_id, process_no);
#endif /* USE_TCP */
		srand(new_seed1);
		fastrand_seed(rand());
		srandom(new_seed2+time(0));
		shm_malloc_on_fork();
#ifdef PROFILING
		monstartup((u_long) &_start, (u_long) &etext);
#endif
#ifdef FORK_DONT_WAIT
		/* record pid twice to avoid the child using it, before
		 * parent gets a chance to set it*/
		pt[process_no].pid=getpid();
#else
		/* wait for parent to get out of critical zone.
		 * this is actually relevant as the parent updates
		 * the pt & process_count. */
		lock_get(process_lock);
		lock_release(process_lock);
#endif
		#ifdef USE_TCP
			if (make_sock && !tcp_disable){
				close(sockfd[0]);
				unix_tcp_sock=sockfd[1];
			}
		#endif		
		if ((child_id!=PROC_NOCHLDINIT) && (init_child(child_id) < 0)) {
			LOG(L_ERR, "ERROR: fork_process(): init_child failed for "
					" process %d, pid %d, \"%s\"\n", process_no,
					pt[process_no].pid, pt[process_no].desc);
			return -1;
		}
		return pid;
	} else {
		/* parent */
		(*process_count)++;
#ifdef FORK_DONT_WAIT
		lock_release(process_lock);
#endif
		/* add the process to the list in shm */
		pt[child_process_no].pid=pid;
		if (desc){
			strncpy(pt[child_process_no].desc, desc, MAX_PT_DESC);
		}
		#ifdef USE_TCP
			if (make_sock && !tcp_disable){
				close(sockfd[1]);
				pt[child_process_no].unix_sock=sockfd[0];
				pt[child_process_no].idx=-1; /* this is not a "tcp" process*/
			}
		#endif
#ifdef FORK_DONT_WAIT
#else
		lock_release(process_lock);
#endif
		ret=pid;
		goto end;
	}
error:
#ifdef USE_TCP
	if (sockfd[0]!=-1) close(sockfd[0]);
	if (sockfd[1]!=-1) close(sockfd[1]);
#endif
end:
	return ret;
}

/**
 * Forks a new TCP process.
 * @param desc - text description for the process table
 * @param r - index in the tcp_children array
 * @param *reader_fd_1 - pointer to return the reader_fd[1]
 * @returns the pid of the new process
 */
#ifdef USE_TCP
int fork_tcp_process(int child_id, char *desc, int r, int *reader_fd_1)
{
	int pid, child_process_no;
	int sockfd[2];
	int reader_fd[2]; /* for comm. with the tcp children read  */
	int ret;
	int i;
	unsigned int new_seed1;
	unsigned int new_seed2;
	
	/* init */
	sockfd[0]=sockfd[1]=-1;
	reader_fd[0]=reader_fd[1]=-1;
	ret=-1;
	
	if (!is_main){
		 LOG(L_CRIT, "BUG: fork_tcp_process() called from a non \"main\" "
				 	"process\n");
		 goto error;
	 }
	 if (tcp_main_pid){
		 LOG(L_CRIT, "BUG: fork_tcp_process(..., 1) called _after_ starting"
				 	" tcp main\n");
		 goto error;
	 }
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd)<0){
		LOG(L_ERR, "ERROR: fork_tcp_process(): socketpair failed: %s\n",
					strerror(errno));
		goto error;
	}
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, reader_fd)<0){
		LOG(L_ERR, "ERROR: fork_tcp_process(): socketpair failed: %s\n",
					strerror(errno));
		goto error;
	}
	if (tcp_fix_child_sockets(reader_fd)<0){
		LOG(L_ERR, "ERROR: fork_tcp_process(): failed to set non blocking"
					"on child sockets\n");
		/* continue, it's not critical (it will go slower under
		 * very high connection rates) */
	}
	lock_get(process_lock);
	/* set the local process_no */
	if (*process_count>=estimated_proc_no) {
		LOG(L_CRIT, "ERROR: fork_tcp_process(): Process limit of %d exceeded."
					" Simulating fork fail\n", estimated_proc_no);
		lock_release(process_lock);
		goto error;
	}
	
	
	child_process_no = *process_count;
	new_seed1=rand();
	new_seed2=random();
	pid = fork();
	if (pid<0) {
		lock_release(process_lock);
		ret=pid;
		goto end;
	}
	if (pid==0){
		is_main=0; /* a forked process cannot be the "main" one */
		process_no=child_process_no;
		/* close unneeded unix sockets */
		close_extra_socks(child_id, process_no);
		/* same for unneeded tcp_children <-> tcp_main unix socks */
		for (i=0; i<r; i++){
			if (tcp_children[i].unix_sock>=0){
				close(tcp_children[i].unix_sock);
				/* tcp_children is per process, so it's safe to change
				 * the unix_sock to -1 */
				tcp_children[i].unix_sock=-1;
			}
		}
		srand(new_seed1);
		fastrand_seed(rand());
		srandom(new_seed2+time(0));
		shm_malloc_on_fork();
#ifdef PROFILING
		monstartup((u_long) &_start, (u_long) &etext);
#endif
#ifdef FORK_DONT_WAIT
		/* record pid twice to avoid the child using it, before
-		 * parent gets a chance to set it*/
		pt[process_no].pid=getpid();
#else
		/* wait for parent to get out of critical zone */
		lock_get(process_lock);
		lock_release(process_lock);
#endif
		close(sockfd[0]);
		unix_tcp_sock=sockfd[1];
		close(reader_fd[0]);
		if (reader_fd_1) *reader_fd_1=reader_fd[1];
		if ((child_id!=PROC_NOCHLDINIT) && (init_child(child_id) < 0)) {
			LOG(L_ERR, "ERROR: fork_tcp_process(): init_child failed for "
					"process %d, pid %d, \"%s\"\n", process_no, 
					pt[process_no].pid, pt[process_no].desc);
			return -1;
		}
		return pid;
	} else {
		/* parent */
		(*process_count)++;
#ifdef FORK_DONT_WAIT
		lock_release(process_lock);
#endif
		/* add the process to the list in shm */
		pt[child_process_no].pid=pid;
		pt[child_process_no].unix_sock=sockfd[0];
		pt[child_process_no].idx=r;
		if (desc){
			snprintf(pt[child_process_no].desc, MAX_PT_DESC, "%s child=%d", 
						desc, r);
		}
#ifdef FORK_DONT_WAIT
#else
		lock_release(process_lock);
#endif
		
		close(sockfd[1]);
		close(reader_fd[1]);
		
		tcp_children[r].pid=pid;
		tcp_children[r].proc_no=child_process_no;
		tcp_children[r].busy=0;
		tcp_children[r].n_reqs=0;
		tcp_children[r].unix_sock=reader_fd[0];
		
		ret=pid;
		goto end;
	}
error:
	if (sockfd[0]!=-1) close(sockfd[0]);
	if (sockfd[1]!=-1) close(sockfd[1]);
	if (reader_fd[0]!=-1) close(reader_fd[0]);
	if (reader_fd[1]!=-1) close(reader_fd[1]);
end:
	return ret;
}
#endif

#ifdef PKG_MALLOC
/* Dumps pkg memory status.
 * Per-child process callback that is called
 * when mem_dump_pkg cfg var is changed.
 */
void mem_dump_pkg_cb(str *name)
{
	int	old_memlog;

	if (cfg_get(core, core_cfg, mem_dump_pkg) == my_pid()) {
		/* set memlog to ALERT level to force
		printing the log messages */
		old_memlog = memlog;
		memlog = L_ALERT;

		LOG(memlog, "Memory status (pkg) of process %d:\n", my_pid());
		pkg_status();

		memlog = old_memlog;
	}
}
#endif

#ifdef SHM_MEM
/* Dumps shm memory status.
 * fixup function that is called
 * when mem_dump_shm cfg var is set.
 */
int mem_dump_shm_fixup(void *handle, str *name, void **val)
{
	int	old_memlog;

	if ((long)(void*)(*val)) {
		/* set memlog to ALERT level to force
		printing the log messages */
		old_memlog = memlog;
		memlog = L_ALERT;

		LOG(memlog, "Memory status (shm)\n");
		shm_status();

		memlog = old_memlog;
		*val = (void*)(long)0;
	}
	return 0;
}
#endif
