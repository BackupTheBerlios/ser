/*
 * $Id: tcp_main.c,v 1.124 2008/01/10 15:18:45 andrei Exp $
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
 *  2002-11-29  created by andrei
 *  2002-12-11  added tcp_send (andrei)
 *  2003-01-20  locking fixes, hashtables (andrei)
 *  2003-02-20  s/lock_t/gen_lock_t/ to avoid a conflict on solaris (andrei)
 *  2003-02-25  Nagle is disabled if -DDISABLE_NAGLE (andrei)
 *  2003-03-29  SO_REUSEADDR before calling bind to allow
 *              server restart, Nagle set on the (hopefuly) 
 *              correct socket (jiri)
 *  2003-03-31  always try to find the corresponding tcp listen socket for
 *               a temp. socket and store in in *->bind_address: added
 *               find_tcp_si, modified tcpconn_connect (andrei)
 *  2003-04-14  set sockopts to TOS low delay (andrei)
 *  2003-06-30  moved tcp new connect checking & handling to
 *               handle_new_connect (andrei)
 *  2003-07-09  tls_close called before closing the tcp connection (andrei)
 *  2003-10-24  converted to the new socket_info lists (andrei)
 *  2003-10-27  tcp port aliases support added (andrei)
 *  2003-11-04  always lock before manipulating refcnt; sendchild
 *              does not inc refcnt by itself anymore (andrei)
 *  2003-11-07  different unix sockets are used for fd passing
 *              to/from readers/writers (andrei)
 *  2003-11-17  handle_new_connect & tcp_connect will close the 
 *              new socket if tcpconn_new return 0 (e.g. out of mem) (andrei)
 *  2003-11-28  tcp_blocking_write & tcp_blocking_connect added (andrei)
 *  2004-11-08  dropped find_tcp_si and replaced with find_si (andrei)
 *  2005-06-07  new tcp optimized code, supports epoll (LT), sigio + real time
 *               signals, poll & select (andrei)
 *  2005-06-26  *bsd kqueue support (andrei)
 *  2005-07-04  solaris /dev/poll support (andrei)
 *  2005-07-08  tcp_max_connections, tcp_connection_lifetime, don't accept
 *               more connections if tcp_max_connections is exceeded (andrei)
 *  2005-10-21  cleanup all the open connections on exit
 *              decrement the no. of open connections on timeout too    (andrei) *  2006-01-30  queue send_fd request and execute them at the end of the
 *              poll loop  (#ifdef) (andrei)
 *              process all children requests, before attempting to send
 *              them new stuff (fixes some deadlocks) (andrei)
 *  2006-02-03  timers are run only once per s (andrei)
 *              tcp children fds can be non-blocking; send fds are queued on
 *              EAGAIN; lots of bug fixes (andrei)
 *  2006-02-06  better tcp_max_connections checks, tcp_connections_no moved to
 *              shm (andrei)
 *  2006-04-12  tcp_send() changed to use struct dest_info (andrei)
 *  2006-11-02  switched to atomic ops for refcnt, locking improvements 
 *               (andrei)
 *  2006-11-04  switched to raw ticks (to fix conversion errors which could
 *               result in inf. lifetime) (andrei)
 *  2007-07-25  tcpconn_connect can now bind the socket on a specified
 *                source addr/port (andrei)
 *  2007-07-26   tcp_send() and tcpconn_get() can now use a specified source
 *                addr./port (andrei)
 *  2007-08-23   getsockname() for INADDR_ANY(SI_IS_ANY) sockets (andrei)
 *  2007-08-27   split init_sock_opt into a lightweight init_sock_opt_accept() 
 *               used when accepting connections and init_sock_opt used for 
 *               connect/ new sockets (andrei)
 *  2007-11-22  always add the connection & clear the coresponding flags before
 *               io_watch_add-ing its fd - it's safer this way (andrei)
 *  2007-11-26  improved tcp timers: switched to local_timer (andrei)
 *  2007-11-27  added send fd cache and reader fd reuse (andrei)
 *  2007-11-28  added support for TCP_DEFER_ACCEPT, KEEPALIVE, KEEPINTVL,
 *               KEEPCNT, QUICKACK, SYNCNT, LINGER2 (andrei)
 *  2007-12-04  support for queueing write requests (andrei)
 *  2007-12-12  destroy connection asap on wbuf. timeout (andrei)
 *  2007-12-13  changed the refcnt and destroy scheme, now refcnt is 1 if
 *                linked into the hash tables (was 0) (andrei)
 *  2007-12-21  support for pending connects (connections are added to the
 *               hash immediately and writes on them are buffered) (andrei)
 */


#ifdef USE_TCP


#ifndef SHM_MEM
#error "shared memory support needed (add -DSHM_MEM to Makefile.defs)"
#endif

#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/uio.h>  /* writev*/
#include <netdb.h>
#include <stdlib.h> /*exit() */

#include <unistd.h>

#include <errno.h>
#include <string.h>

#ifdef HAVE_SELECT
#include <sys/select.h>
#endif
#include <sys/poll.h>


#include "ip_addr.h"
#include "pass_fd.h"
#include "tcp_conn.h"
#include "globals.h"
#include "pt.h"
#include "locking.h"
#include "mem/mem.h"
#include "mem/shm_mem.h"
#include "timer.h"
#include "sr_module.h"
#include "tcp_server.h"
#include "tcp_init.h"
#include "tsend.h"
#include "timer_ticks.h"
#include "local_timer.h"
#ifdef CORE_TLS
#include "tls/tls_server.h"
#define tls_loaded() 1
#else
#include "tls_hooks_init.h"
#include "tls_hooks.h"
#endif

#include "tcp_info.h"
#include "tcp_options.h"
#include "ut.h"
#include "cfg/cfg_struct.h"

#define local_malloc pkg_malloc
#define local_free   pkg_free

#define HANDLE_IO_INLINE
#include "io_wait.h"
#include <fcntl.h> /* must be included after io_wait.h if SIGIO_RT is used */


#define TCP_PASS_NEW_CONNECTION_ON_DATA /* don't pass a new connection
										   immediately to a child, wait for
										   some data on it first */
#define TCP_LISTEN_BACKLOG 1024
#define SEND_FD_QUEUE /* queue send fd requests on EAGAIN, instead of sending 
							them immediately */
#define TCP_CHILD_NON_BLOCKING 
#ifdef SEND_FD_QUEUE
#ifndef TCP_CHILD_NON_BLOCKING
#define TCP_CHILD_NON_BLOCKING
#endif
#define MAX_SEND_FD_QUEUE_SIZE	tcp_main_max_fd_no
#define SEND_FD_QUEUE_SIZE		128  /* initial size */
#define MAX_SEND_FD_RETRIES		96	 /* FIXME: not used for now */
#define SEND_FD_QUEUE_TIMEOUT	MS_TO_TICKS(2000)  /* 2 s */
#endif

/* maximum accepted lifetime (maximum possible is  ~ MAXINT/2) */
#define MAX_TCP_CON_LIFETIME	((1U<<(sizeof(ticks_t)*8-1))-1)
/* minimum interval local_timer_run() is allowed to run, in ticks */
#define TCPCONN_TIMEOUT_MIN_RUN 1  /* once per tick */
#define TCPCONN_WAIT_TIMEOUT 1 /* 1 tick */

#ifdef TCP_BUF_WRITE
#define TCP_WBUF_SIZE	1024 /* FIXME: after debugging switch to 16-32k */
static unsigned int* tcp_total_wq=0;
#endif


enum fd_types { F_NONE, F_SOCKINFO /* a tcp_listen fd */,
				F_TCPCONN, F_TCPCHILD, F_PROC };


#ifdef TCP_FD_CACHE

#define TCP_FD_CACHE_SIZE 8

struct fd_cache_entry{
	struct tcp_connection* con;
	int id;
	int fd;
};


static struct fd_cache_entry fd_cache[TCP_FD_CACHE_SIZE];
#endif /* TCP_FD_CACHE */

static int is_tcp_main=0;

int tcp_accept_aliases=0; /* by default don't accept aliases */
/* flags used for adding new aliases */
int tcp_alias_flags=TCP_ALIAS_FORCE_ADD;
/* flags used for adding the default aliases of a new tcp connection */
int tcp_new_conn_alias_flags=TCP_ALIAS_REPLACE;
int tcp_connect_timeout=DEFAULT_TCP_CONNECT_TIMEOUT;
int tcp_send_timeout=DEFAULT_TCP_SEND_TIMEOUT;
int tcp_con_lifetime=DEFAULT_TCP_CONNECTION_LIFETIME;
enum poll_types tcp_poll_method=0; /* by default choose the best method */
int tcp_max_connections=DEFAULT_TCP_MAX_CONNECTIONS;
int tcp_main_max_fd_no=0;

static union sockaddr_union tcp_source_ipv4_addr; /* saved bind/srv v4 addr. */
static union sockaddr_union* tcp_source_ipv4=0;
#ifdef USE_IPV6
static union sockaddr_union tcp_source_ipv6_addr; /* saved bind/src v6 addr. */
static union sockaddr_union* tcp_source_ipv6=0;
#endif

static int* tcp_connections_no=0; /* current open connections */

/* connection hash table (after ip&port) , includes also aliases */
struct tcp_conn_alias** tcpconn_aliases_hash=0;
/* connection hash table (after connection id) */
struct tcp_connection** tcpconn_id_hash=0;
gen_lock_t* tcpconn_lock=0;

struct tcp_child* tcp_children;
static int* connection_id=0; /*  unique for each connection, used for 
								quickly finding the corresponding connection
								for a reply */
int unix_tcp_sock;

static int tcp_proto_no=-1; /* tcp protocol number as returned by
							   getprotobyname */

static io_wait_h io_h;

static struct local_timer tcp_main_ltimer;
static ticks_t tcp_main_prev_ticks;


static ticks_t tcpconn_main_timeout(ticks_t , struct timer_ln* , void* );

inline static int _tcpconn_add_alias_unsafe(struct tcp_connection* c, int port,
										struct ip_addr* l_ip, int l_port,
										int flags);



/* sets source address used when opening new sockets and no source is specified
 *  (by default the address is choosen by the kernel)
 * Should be used only on init.
 * returns -1 on error */
int tcp_set_src_addr(struct ip_addr* ip)
{
	switch (ip->af){
		case AF_INET:
			ip_addr2su(&tcp_source_ipv4_addr, ip, 0);
			tcp_source_ipv4=&tcp_source_ipv4_addr;
			break;
		#ifdef USE_IPV6
		case AF_INET6:
			ip_addr2su(&tcp_source_ipv6_addr, ip, 0);
			tcp_source_ipv6=&tcp_source_ipv6_addr;
			break;
		#endif
		default:
			return -1;
	}
	return 0;
}



static inline int init_sock_keepalive(int s)
{
	int optval;
	
#ifdef HAVE_SO_KEEPALIVE
	if (tcp_options.keepalive){
		optval=1;
		if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval,
						sizeof(optval))<0){
			LOG(L_WARN, "WARNING: init_sock_keepalive: failed to enable"
						" SO_KEEPALIVE: %s\n", strerror(errno));
			return -1;
		}
	}
#endif
#ifdef HAVE_TCP_KEEPINTVL
	if (tcp_options.keepintvl){
		optval=tcp_options.keepintvl;
		if (setsockopt(s, IPPROTO_TCP, TCP_KEEPINTVL, &optval,
						sizeof(optval))<0){
			LOG(L_WARN, "WARNING: init_sock_keepalive: failed to set"
						" keepalive probes interval: %s\n", strerror(errno));
		}
	}
#endif
#ifdef HAVE_TCP_KEEPIDLE
	if (tcp_options.keepidle){
		optval=tcp_options.keepidle;
		if (setsockopt(s, IPPROTO_TCP, TCP_KEEPIDLE, &optval,
						sizeof(optval))<0){
			LOG(L_WARN, "WARNING: init_sock_keepalive: failed to set"
						" keepalive idle interval: %s\n", strerror(errno));
		}
	}
#endif
#ifdef HAVE_TCP_KEEPCNT
	if (tcp_options.keepcnt){
		optval=tcp_options.keepcnt;
		if (setsockopt(s, IPPROTO_TCP, TCP_KEEPCNT, &optval,
						sizeof(optval))<0){
			LOG(L_WARN, "WARNING: init_sock_keepalive: failed to set"
						" maximum keepalive count: %s\n", strerror(errno));
		}
	}
#endif
	return 0;
}



/* set all socket/fd options for new sockets (e.g. before connect): 
 *  disable nagle, tos lowdelay, reuseaddr, non-blocking
 *
 * return -1 on error */
static int init_sock_opt(int s)
{
	int flags;
	int optval;
	
#ifdef DISABLE_NAGLE
	flags=1;
	if ( (tcp_proto_no!=-1) && (setsockopt(s, tcp_proto_no , TCP_NODELAY,
					&flags, sizeof(flags))<0) ){
		LOG(L_WARN, "WARNING: init_sock_opt: could not disable Nagle: %s\n",
				strerror(errno));
	}
#endif
	/* tos*/
	optval = tos;
	if (setsockopt(s, IPPROTO_IP, IP_TOS, (void*)&optval,sizeof(optval)) ==-1){
		LOG(L_WARN, "WARNING: init_sock_opt: setsockopt tos: %s\n",
				strerror(errno));
		/* continue since this is not critical */
	}
#if  !defined(TCP_DONT_REUSEADDR) 
	optval=1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
						(void*)&optval, sizeof(optval))==-1){
		LOG(L_ERR, "ERROR: setsockopt SO_REUSEADDR %s\n",
				strerror(errno));
		/* continue, not critical */
	}
#endif /* !TCP_DONT_REUSEADDR */
#ifdef HAVE_TCP_SYNCNT
	if (tcp_options.syncnt){
		optval=tcp_options.syncnt;
		if (setsockopt(s, IPPROTO_TCP, TCP_SYNCNT, &optval,
						sizeof(optval))<0){
			LOG(L_WARN, "WARNING: init_sock_opt: failed to set"
						" maximum SYN retr. count: %s\n", strerror(errno));
		}
	}
#endif
#ifdef HAVE_TCP_LINGER2
	if (tcp_options.linger2){
		optval=tcp_options.linger2;
		if (setsockopt(s, IPPROTO_TCP, TCP_LINGER2, &optval,
						sizeof(optval))<0){
			LOG(L_WARN, "WARNING: init_sock_opt: failed to set"
						" maximum LINGER2 timeout: %s\n", strerror(errno));
		}
	}
#endif
#ifdef HAVE_TCP_QUICKACK
	if (tcp_options.delayed_ack){
		optval=0; /* reset quick ack => delayed ack */
		if (setsockopt(s, IPPROTO_TCP, TCP_QUICKACK, &optval,
						sizeof(optval))<0){
			LOG(L_WARN, "WARNING: init_sock_opt: failed to reset"
						" TCP_QUICKACK: %s\n", strerror(errno));
		}
	}
#endif /* HAVE_TCP_QUICKACK */
	init_sock_keepalive(s);
	
	/* non-blocking */
	flags=fcntl(s, F_GETFL);
	if (flags==-1){
		LOG(L_ERR, "ERROR: init_sock_opt: fnctl failed: (%d) %s\n",
				errno, strerror(errno));
		goto error;
	}
	if (fcntl(s, F_SETFL, flags|O_NONBLOCK)==-1){
		LOG(L_ERR, "ERROR: init_sock_opt: fcntl: set non-blocking failed:"
				" (%d) %s\n", errno, strerror(errno));
		goto error;
	}
	return 0;
error:
	return -1;
}



/* set all socket/fd options for "accepted" sockets 
 *  only nonblocking is set since the rest is inherited from the
 *  "parent" (listening) socket
 *  Note: setting O_NONBLOCK is required on linux but it's not needed on
 *        BSD and possibly solaris (where the flag is inherited from the 
 *        parent socket). However since there is no standard document 
 *        requiring a specific behaviour in this case it's safer to always set
 *        it (at least for now)  --andrei
 *  TODO: check on which OSes  O_NONBLOCK is inherited and make this 
 *        function a nop.
 *
 * return -1 on error */
static int init_sock_opt_accept(int s)
{
	int flags;
	
	/* non-blocking */
	flags=fcntl(s, F_GETFL);
	if (flags==-1){
		LOG(L_ERR, "ERROR: init_sock_opt_accept: fnctl failed: (%d) %s\n",
				errno, strerror(errno));
		goto error;
	}
	if (fcntl(s, F_SETFL, flags|O_NONBLOCK)==-1){
		LOG(L_ERR, "ERROR: init_sock_opt_accept: "
					"fcntl: set non-blocking failed: (%d) %s\n",
					errno, strerror(errno));
		goto error;
	}
	return 0;
error:
	return -1;
}



/* blocking connect on a non-blocking fd; it will timeout after
 * tcp_connect_timeout 
 * if BLOCKING_USE_SELECT and HAVE_SELECT are defined it will internally
 * use select() instead of poll (bad if fd > FD_SET_SIZE, poll is preferred)
 */
static int tcp_blocking_connect(int fd, const struct sockaddr *servaddr,
								socklen_t addrlen)
{
	int n;
#if defined(HAVE_SELECT) && defined(BLOCKING_USE_SELECT)
	fd_set sel_set;
	fd_set orig_set;
	struct timeval timeout;
#else
	struct pollfd pf;
#endif
	int elapsed;
	int to;
	int ticks;
	int err;
	unsigned int err_len;
	int poll_err;
	
	poll_err=0;
	to=tcp_connect_timeout;
	ticks=get_ticks();
again:
	n=connect(fd, servaddr, addrlen);
	if (n==-1){
		if (errno==EINTR){
			elapsed=(get_ticks()-ticks)*TIMER_TICK;
			if (elapsed<to)		goto again;
			else goto error_timeout;
		}
		if (errno!=EINPROGRESS && errno!=EALREADY){
			LOG(L_ERR, "ERROR: tcp_blocking_connect: (%d) %s\n",
					errno, strerror(errno));
			goto error;
		}
	}else goto end;
	
	/* poll/select loop */
#if defined(HAVE_SELECT) && defined(BLOCKING_USE_SELECT)
		FD_ZERO(&orig_set);
		FD_SET(fd, &orig_set);
#else
		pf.fd=fd;
		pf.events=POLLOUT;
#endif
	while(1){
		elapsed=(get_ticks()-ticks)*TIMER_TICK;
		if (elapsed<to)
			to-=elapsed;
		else 
			goto error_timeout;
#if defined(HAVE_SELECT) && defined(BLOCKING_USE_SELECT)
		sel_set=orig_set;
		timeout.tv_sec=to;
		timeout.tv_usec=0;
		n=select(fd+1, 0, &sel_set, 0, &timeout);
#else
		n=poll(&pf, 1, to*1000);
#endif
		if (n<0){
			if (errno==EINTR) continue;
			LOG(L_ERR, "ERROR: tcp_blocking_connect: poll/select failed:"
					" (%d) %s\n", errno, strerror(errno));
			goto error;
		}else if (n==0) /* timeout */ continue;
#if defined(HAVE_SELECT) && defined(BLOCKING_USE_SELECT)
		if (FD_ISSET(fd, &sel_set))
#else
		if (pf.revents&(POLLERR|POLLHUP|POLLNVAL)){ 
			LOG(L_ERR, "ERROR: tcp_blocking_connect: poll error: flags %x\n",
					pf.revents);
			poll_err=1;
		}
#endif
		{
			err_len=sizeof(err);
			getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &err_len);
			if ((err==0) && (poll_err==0)) goto end;
			if (err!=EINPROGRESS && err!=EALREADY){
				LOG(L_ERR, "ERROR: tcp_blocking_connect: SO_ERROR (%d) %s\n",
						err, strerror(err));
				goto error;
			}
		}
	}
error_timeout:
	/* timeout */
	LOG(L_ERR, "ERROR: tcp_blocking_connect: timeout %d s elapsed from %d s\n",
			elapsed, tcp_connect_timeout);
error:
	return -1;
end:
	return 0;
}



inline static int _tcpconn_write_nb(int fd, struct tcp_connection* c,
									char* buf, int len);


#ifdef TCP_BUF_WRITE


/* unsafe version */
#define _wbufq_empty(con) ((con)->wbuf_q.first==0)
/* unsafe version */
#define _wbufq_non_empty(con) ((con)->wbuf_q.first!=0)


/* unsafe version, call while holding the connection write lock */
inline static int _wbufq_add(struct  tcp_connection* c, char* data, 
							unsigned int size)
{
	struct tcp_wbuffer_queue* q;
	struct tcp_wbuffer* wb;
	unsigned int last_free;
	unsigned int wb_size;
	unsigned int crt_size;
	ticks_t t;
	
	q=&c->wbuf_q;
	t=get_ticks_raw();
	if (unlikely(	((q->queued+size)>tcp_options.tcpconn_wq_max) ||
					((*tcp_total_wq+size)>tcp_options.tcp_wq_max) ||
					(q->first &&
					TICKS_LT(q->wr_timeout, t)) )){
		LOG(L_ERR, "ERROR: wbufq_add(%d bytes): write queue full or timeout "
					" (%d, total %d, last write %d s ago)\n",
					size, q->queued, *tcp_total_wq,
					TICKS_TO_S(t-q->wr_timeout-tcp_options.tcp_wq_timeout));
		goto error;
	}
	
	if (unlikely(q->last==0)){
		wb_size=MAX_unsigned(TCP_WBUF_SIZE, size);
		wb=shm_malloc(sizeof(*wb)+wb_size-1);
		if (unlikely(wb==0))
			goto error;
		wb->b_size=wb_size;
		wb->next=0;
		q->last=wb;
		q->first=wb;
		q->last_used=0;
		q->offset=0;
		q->wr_timeout=get_ticks_raw()+tcp_options.tcp_wq_timeout;
	}else{
		wb=q->last;
	}
	
	while(size){
		last_free=wb->b_size-q->last_used;
		if (last_free==0){
			wb_size=MAX_unsigned(TCP_WBUF_SIZE, size);
			wb=shm_malloc(sizeof(*wb)+wb_size-1);
			if (unlikely(wb==0))
				goto error;
			wb->b_size=wb_size;
			wb->next=0;
			q->last->next=wb;
			q->last=wb;
			q->last_used=0;
			last_free=wb->b_size;
		}
		crt_size=MIN_unsigned(last_free, size);
		memcpy(wb->buf, data, crt_size);
		q->last_used+=crt_size;
		size-=crt_size;
		data+=crt_size;
		q->queued+=crt_size;
		atomic_add_int((int*)tcp_total_wq, crt_size);
	}
	return 0;
error:
	return -1;
}



/* unsafe version, call while holding the connection write lock
 * inserts data at the beginning, it ignores the max queue size checks and
 * the timeout (use sparingly)
 * Note: it should never be called on a write buffer after wbufq_run() */
inline static int _wbufq_insert(struct  tcp_connection* c, char* data, 
							unsigned int size)
{
	struct tcp_wbuffer_queue* q;
	struct tcp_wbuffer* wb;
	
	q=&c->wbuf_q;
	if (likely(q->first==0)) /* if empty, use wbufq_add */
		return _wbufq_add(c, data, size);
	
	if (unlikely((*tcp_total_wq+size)>tcp_options.tcp_wq_max)){
		LOG(L_ERR, "ERROR: wbufq_insert(%d bytes): write queue full or timeout"
					" (%d, total %d, last write %d s ago)\n",
					size, q->queued, *tcp_total_wq,
					TICKS_TO_S(get_ticks_raw()-q->wr_timeout-
										tcp_options.tcp_wq_timeout));
		goto error;
	}
	if (unlikely(q->offset)){
		LOG(L_CRIT, "BUG: wbufq_insert: non-null offset %d (bad call, should"
				"never be called after the wbufq_run())\n", q->offset);
		goto error;
	}
	if ((q->first==q->last) && ((q->last->b_size-q->last_used)>=size)){
		/* one block with enough space in it for size bytes */
		memmove(q->first->buf+size, q->first->buf, size);
		memcpy(q->first->buf, data, size);
		q->last_used+=size;
	}else{
		/* create a size bytes block directly */
		wb=shm_malloc(sizeof(*wb)+size-1);
		if (unlikely(wb==0))
			goto error;
		wb->b_size=size;
		/* insert it */
		wb->next=q->first;
		q->first=wb;
		memcpy(wb->buf, data, size);
	}
	
	q->queued+=size;
	atomic_add_int((int*)tcp_total_wq, size);
	return 0;
error:
	return -1;
}



/* unsafe version, call while holding the connection write lock */
inline static void _wbufq_destroy( struct  tcp_wbuffer_queue* q)
{
	struct tcp_wbuffer* wb;
	struct tcp_wbuffer* next_wb;
	int unqueued;
	
	unqueued=0;
	if (likely(q->first)){
		wb=q->first;
		do{
			next_wb=wb->next;
			unqueued+=(wb==q->last)?q->last_used:wb->b_size;
			if (wb==q->first)
				unqueued-=q->offset;
			shm_free(wb);
			wb=next_wb;
		}while(wb);
	}
	memset(q, 0, sizeof(*q));
	atomic_add_int((int*)tcp_total_wq, -unqueued);
}



/* tries to empty the queue  (safe version, c->write_lock must not be hold)
 * returns -1 on error, bytes written on success (>=0) 
 * if the whole queue is emptied => sets *empty*/
inline static int wbufq_run(int fd, struct tcp_connection* c, int* empty)
{
	struct tcp_wbuffer_queue* q;
	struct tcp_wbuffer* wb;
	int n;
	int ret;
	int block_size;
	ticks_t t;
	char* buf;
	
	*empty=0;
	ret=0;
	t=get_ticks_raw();
	lock_get(&c->write_lock);
	q=&c->wbuf_q;
	while(q->first){
		block_size=((q->first==q->last)?q->last_used:q->first->b_size)-
						q->offset;
		buf=q->first->buf+q->offset;
		n=_tcpconn_write_nb(fd, c, buf, block_size);
		if (likely(n>0)){
			ret+=n;
			if (likely(n==block_size)){
				wb=q->first;
				q->first=q->first->next; 
				shm_free(wb);
				q->offset=0;
				q->queued-=block_size;
				atomic_add_int((int*)tcp_total_wq, -block_size);
			}else{
				q->offset+=n;
				q->queued-=n;
				atomic_add_int((int*)tcp_total_wq, -n);
				break;
			}
			q->wr_timeout=t+tcp_options.tcp_wq_timeout;
			c->state=S_CONN_OK;
		}else{
			if (n<0){
				/* EINTR is handled inside _tcpconn_write_nb */
				if (!(errno==EAGAIN || errno==EWOULDBLOCK)){
					ret=-1;
					LOG(L_ERR, "ERROR: wbuf_runq: %s [%d]\n",
						strerror(errno), errno);
				}
			}
			break;
		}
	}
	if (likely(q->first==0)){
		q->last=0;
		q->last_used=0;
		q->offset=0;
		*empty=1;
	}
	if (unlikely(c->state==S_CONN_CONNECT && (ret>0)))
			c->state=S_CONN_OK;
	lock_release(&c->write_lock);
	return ret;
}

#endif /* TCP_BUF_WRITE */



#if 0
/* blocking write even on non-blocking sockets 
 * if TCP_TIMEOUT will return with error */
static int tcp_blocking_write(struct tcp_connection* c, int fd, char* buf,
								unsigned int len)
{
	int n;
	fd_set sel_set;
	struct timeval timeout;
	int ticks;
	int initial_len;
	
	initial_len=len;
again:
	
	n=send(fd, buf, len,
#ifdef HAVE_MSG_NOSIGNAL
			MSG_NOSIGNAL
#else
			0
#endif
		);
	if (n<0){
		if (errno==EINTR)	goto again;
		else if (errno!=EAGAIN && errno!=EWOULDBLOCK){
			LOG(L_ERR, "tcp_blocking_write: failed to send: (%d) %s\n",
					errno, strerror(errno));
			goto error;
		}
	}else if (n<len){
		/* partial write */
		buf+=n;
		len-=n;
	}else{
		/* success: full write */
		goto end;
	}
	while(1){
		FD_ZERO(&sel_set);
		FD_SET(fd, &sel_set);
		timeout.tv_sec=tcp_send_timeout;
		timeout.tv_usec=0;
		ticks=get_ticks();
		n=select(fd+1, 0, &sel_set, 0, &timeout);
		if (n<0){
			if (errno==EINTR) continue; /* signal, ignore */
			LOG(L_ERR, "ERROR: tcp_blocking_write: select failed: "
					" (%d) %s\n", errno, strerror(errno));
			goto error;
		}else if (n==0){
			/* timeout */
			if (get_ticks()-ticks>=tcp_send_timeout){
				LOG(L_ERR, "ERROR: tcp_blocking_write: send timeout (%d)\n",
						tcp_send_timeout);
				goto error;
			}
			continue;
		}
		if (FD_ISSET(fd, &sel_set)){
			/* we can write again */
			goto again;
		}
	}
error:
		return -1;
end:
		return initial_len;
}
#endif



struct tcp_connection* tcpconn_new(int sock, union sockaddr_union* su,
									union sockaddr_union* local_addr,
									struct socket_info* ba, int type, 
									int state)
{
	struct tcp_connection *c;
	
	c=(struct tcp_connection*)shm_malloc(sizeof(struct tcp_connection));
	if (c==0){
		LOG(L_ERR, "ERROR: tcpconn_new: mem. allocation failure\n");
		goto error;
	}
	memset(c, 0, sizeof(struct tcp_connection)); /* zero init */
	c->s=sock;
	c->fd=-1; /* not initialized */
	if (lock_init(&c->write_lock)==0){
		LOG(L_ERR, "ERROR: tcpconn_new: init lock failed\n");
		goto error;
	}
	
	c->rcv.src_su=*su;
	
	atomic_set(&c->refcnt, 0);
	local_timer_init(&c->timer, tcpconn_main_timeout, c, 0);
	su2ip_addr(&c->rcv.src_ip, su);
	c->rcv.src_port=su_getport(su);
	c->rcv.bind_address=ba;
	if (likely(local_addr)){
		su2ip_addr(&c->rcv.dst_ip, local_addr);
		c->rcv.dst_port=su_getport(local_addr);
	}else if (ba){
		c->rcv.dst_ip=ba->address;
		c->rcv.dst_port=ba->port_no;
	}
	print_ip("tcpconn_new: new tcp connection: ", &c->rcv.src_ip, "\n");
	DBG(     "tcpconn_new: on port %d, type %d\n", c->rcv.src_port, type);
	init_tcp_req(&c->req);
	c->id=(*connection_id)++;
	c->rcv.proto_reserved1=0; /* this will be filled before receive_message*/
	c->rcv.proto_reserved2=0;
	c->state=state;
	c->extra_data=0;
#ifdef USE_TLS
	if (type==PROTO_TLS){
		if (tls_tcpconn_init(c, sock)==-1) goto error;
	}else
#endif /* USE_TLS*/
	{
		c->type=PROTO_TCP;
		c->rcv.proto=PROTO_TCP;
		c->timeout=get_ticks_raw()+tcp_con_lifetime;
	}
	c->flags|=F_CONN_REMOVED;
	
	return c;
	
error:
	if (c) shm_free(c);
	return 0;
}



/* do the actual connect, set sock. options a.s.o
 * returns socket on success, -1 on error
 * sets also *res_local_addr, res_si and state (S_CONN_CONNECT for an
 * unfinished connect and S_CONN_OK for a finished one)*/
inline static int tcp_do_connect(	union sockaddr_union* server,
									union sockaddr_union* from,
									int type,
									union sockaddr_union* res_local_addr,
									struct socket_info** res_si,
									enum tcp_conn_states *state
									)
{
	int s;
	union sockaddr_union my_name;
	socklen_t my_name_len;
	struct ip_addr ip;
#ifdef TCP_BUF_WRITE
	int n;
#endif /* TCP_BUF_WRITE */

	s=socket(AF2PF(server->s.sa_family), SOCK_STREAM, 0);
	if (unlikely(s==-1)){
		LOG(L_ERR, "ERROR: tcp_do_connect: socket: (%d) %s\n",
				errno, strerror(errno));
		goto error;
	}
	if (init_sock_opt(s)<0){
		LOG(L_ERR, "ERROR: tcp_do_connect: init_sock_opt failed\n");
		goto error;
	}
	
	if (unlikely(from && bind(s, &from->s, sockaddru_len(*from)) != 0)){
		LOG(L_WARN, "WARNING: tcp_do_connect: binding to source address"
					" failed: %s [%d]\n", strerror(errno), errno);
	}
	*state=S_CONN_OK;
#ifdef TCP_BUF_WRITE
	if (likely(tcp_options.tcp_buf_write)){
again:
		n=connect(s, &server->s, sockaddru_len(*server));
		if (unlikely(n==-1)){
			if (errno==EINTR) goto again;
			if (likely(errno==EINPROGRESS))
				*state=S_CONN_CONNECT;
			else if (errno!=EALREADY){
				LOG(L_ERR, "ERROR: tcp_do_connect: connect: (%d) %s\n",
						errno, strerror(errno));
				goto error;
			}
		}
	}else{
#endif /* TCP_BUF_WRITE */
		if (tcp_blocking_connect(s, &server->s, sockaddru_len(*server))<0){
			LOG(L_ERR, "ERROR: tcp_do_connect: tcp_blocking_connect"
						" failed\n");
			goto error;
		}
#ifdef TCP_BUF_WRITE
	}
#endif /* TCP_BUF_WRITE */
	if (from){
		su2ip_addr(&ip, from);
		if (!ip_addr_any(&ip))
			/* we already know the source ip, skip the sys. call */
			goto find_socket;
	}
	my_name_len=sizeof(my_name);
	if (unlikely(getsockname(s, &my_name.s, &my_name_len)!=0)){
		LOG(L_ERR, "ERROR: tcp_do_connect: getsockname failed: %s(%d)\n",
				strerror(errno), errno);
		*res_si=0;
		goto error;
	}
	from=&my_name; /* update from with the real "from" address */
	su2ip_addr(&ip, &my_name);
find_socket:
#ifdef USE_TLS
	if (unlikely(type==PROTO_TLS))
		*res_si=find_si(&ip, 0, PROTO_TLS);
	else
#endif
		*res_si=find_si(&ip, 0, PROTO_TCP);
	
	if (unlikely(*res_si==0)){
		LOG(L_WARN, "WARNING: tcp_do_connect: could not find corresponding"
				" listening socket, using default...\n");
		if (server->s.sa_family==AF_INET) *res_si=sendipv4_tcp;
#ifdef USE_IPV6
		else *res_si=sendipv6_tcp;
#endif
	}
	*res_local_addr=*from;
	return s;
error:
	if (s!=-1) close(s);
	return -1;
}



struct tcp_connection* tcpconn_connect( union sockaddr_union* server, 
										union sockaddr_union* from,
										int type)
{
	int s;
	struct socket_info* si;
	union sockaddr_union my_name;
	struct tcp_connection* con;
	enum tcp_conn_states state;

	s=-1;
	
	if (*tcp_connections_no >= tcp_max_connections){
		LOG(L_ERR, "ERROR: tcpconn_connect: maximum number of connections"
					" exceeded (%d/%d)\n",
					*tcp_connections_no, tcp_max_connections);
		goto error;
	}
	s=tcp_do_connect(server, from, type, &my_name, &si, &state);
	if (s==-1){
		LOG(L_ERR, "ERROR: tcp_do_connect: failed (%d) %s\n",
				errno, strerror(errno));
		goto error;
	}
	con=tcpconn_new(s, server, &my_name, si, type, state);
	if (con==0){
		LOG(L_ERR, "ERROR: tcp_connect: tcpconn_new failed, closing the "
				 " socket\n");
		goto error;
	}
	return con;
	/*FIXME: set sock idx! */
error:
	if (s!=-1) close(s); /* close the opened socket */
	return 0;
}



#ifdef TCP_CONNECT_WAIT
int tcpconn_finish_connect( struct tcp_connection* c,
												union sockaddr_union* from)
{
	int s;
	int r;
	union sockaddr_union local_addr;
	struct socket_info* si;
	enum tcp_conn_states state;
	struct tcp_conn_alias* a;
	
	s=tcp_do_connect(&c->rcv.src_su, from, c->type, &local_addr, &si, &state);
	if (unlikely(s==-1)){
		LOG(L_ERR, "ERROR: tcpconn_finish_connect: tcp_do_connect for %p"
						" failed\n", c);
		return -1;
	}
	c->rcv.bind_address=si;
	su2ip_addr(&c->rcv.dst_ip, &local_addr);
	c->rcv.dst_port=su_getport(&local_addr);
	/* update aliases if needed */
	if (likely(from==0)){
		/* add aliases */
		TCPCONN_LOCK;
		_tcpconn_add_alias_unsafe(c, c->rcv.src_port, &c->rcv.dst_ip, 0,
													tcp_new_conn_alias_flags);
		_tcpconn_add_alias_unsafe(c, c->rcv.src_port, &c->rcv.dst_ip,
									c->rcv.dst_port, tcp_new_conn_alias_flags);
		TCPCONN_UNLOCK;
	}else if (su_cmp(from, &local_addr)!=1){
		TCPCONN_LOCK;
			/* remove all the aliases except the first one and re-add them
			 * (there shouldn't be more then the 3 default aliases at this 
			 * stage) */
			for (r=1; r<c->aliases; r++){
				a=&c->con_aliases[r];
				tcpconn_listrm(tcpconn_aliases_hash[a->hash], a, next, prev);
			}
			c->aliases=1;
			/* add the local_ip:0 and local_ip:local_port aliases */
			_tcpconn_add_alias_unsafe(c, c->rcv.src_port, &c->rcv.dst_ip,
												0, tcp_new_conn_alias_flags);
			_tcpconn_add_alias_unsafe(c, c->rcv.src_port, &c->rcv.dst_ip,
									c->rcv.dst_port, tcp_new_conn_alias_flags);
		TCPCONN_UNLOCK;
	}
	
	return s;
}
#endif /* TCP_CONNECT_WAIT */



/* adds a tcp connection to the tcpconn hashes
 * Note: it's called _only_ from the tcp_main process */
inline static struct tcp_connection*  tcpconn_add(struct tcp_connection *c)
{
	struct ip_addr zero_ip;

	if (likely(c)){
		ip_addr_mk_any(c->rcv.src_ip.af, &zero_ip);
		c->id_hash=tcp_id_hash(c->id);
		c->aliases=0;
		TCPCONN_LOCK;
		c->flags|=F_CONN_HASHED;
		/* add it at the begining of the list*/
		tcpconn_listadd(tcpconn_id_hash[c->id_hash], c, id_next, id_prev);
		/* set the aliases */
		/* first alias is for (peer_ip, peer_port, 0 ,0) -- for finding
		 *  any connection to peer_ip, peer_port
		 * the second alias is for (peer_ip, peer_port, local_addr, 0) -- for
		 *  finding any conenction to peer_ip, peer_port from local_addr 
		 * the third alias is for (peer_ip, peer_port, local_addr, local_port) 
		 *   -- for finding if a fully specified connection exists */
		_tcpconn_add_alias_unsafe(c, c->rcv.src_port, &zero_ip, 0,
													tcp_new_conn_alias_flags);
		if (likely(c->rcv.dst_ip.af && ! ip_addr_any(&c->rcv.dst_ip))){
			_tcpconn_add_alias_unsafe(c, c->rcv.src_port, &c->rcv.dst_ip, 0,
													tcp_new_conn_alias_flags);
			_tcpconn_add_alias_unsafe(c, c->rcv.src_port, &c->rcv.dst_ip,
									c->rcv.dst_port, tcp_new_conn_alias_flags);
		}
		/* ignore add_alias errors, there are some valid cases when one
		 *  of the add_alias would fail (e.g. first add_alias for 2 connections
		 *   with the same destination but different src. ip*/
		TCPCONN_UNLOCK;
		DBG("tcpconn_add: hashes: %d:%d:%d, %d\n",
												c->con_aliases[0].hash,
												c->con_aliases[1].hash,
												c->con_aliases[2].hash,
												c->id_hash);
		return c;
	}else{
		LOG(L_CRIT, "tcpconn_add: BUG: null connection pointer\n");
		return 0;
	}
}


static inline void _tcpconn_detach(struct tcp_connection *c)
{
	int r;
	tcpconn_listrm(tcpconn_id_hash[c->id_hash], c, id_next, id_prev);
	/* remove all the aliases */
	for (r=0; r<c->aliases; r++)
		tcpconn_listrm(tcpconn_aliases_hash[c->con_aliases[r].hash], 
						&c->con_aliases[r], next, prev);
}



static inline void _tcpconn_free(struct tcp_connection* c)
{
#ifdef TCP_BUF_WRITE
	if (unlikely(_wbufq_non_empty(c)))
		_wbufq_destroy(&c->wbuf_q);
#endif
	lock_destroy(&c->write_lock);
#ifdef USE_TLS
	if (unlikely(c->type==PROTO_TLS)) tls_tcpconn_clean(c);
#endif
	shm_free(c);
}



/* unsafe tcpconn_rm version (nolocks) */
void _tcpconn_rm(struct tcp_connection* c)
{
	_tcpconn_detach(c);
	_tcpconn_free(c);
}



void tcpconn_rm(struct tcp_connection* c)
{
	int r;
	TCPCONN_LOCK;
	tcpconn_listrm(tcpconn_id_hash[c->id_hash], c, id_next, id_prev);
	/* remove all the aliases */
	for (r=0; r<c->aliases; r++)
		tcpconn_listrm(tcpconn_aliases_hash[c->con_aliases[r].hash], 
						&c->con_aliases[r], next, prev);
	TCPCONN_UNLOCK;
	lock_destroy(&c->write_lock);
#ifdef USE_TLS
	if ((c->type==PROTO_TLS)&&(c->extra_data)) tls_tcpconn_clean(c);
#endif
	shm_free(c);
}


/* finds a connection, if id=0 uses the ip addr, port, local_ip and local port
 *  (host byte order) and tries to find the connection that matches all of
 *   them. Wild cards can be used for local_ip and local_port (a 0 filled
 *   ip address and/or a 0 local port).
 * WARNING: unprotected (locks) use tcpconn_get unless you really
 * know what you are doing */
struct tcp_connection* _tcpconn_find(int id, struct ip_addr* ip, int port,
										struct ip_addr* l_ip, int l_port)
{

	struct tcp_connection *c;
	struct tcp_conn_alias* a;
	unsigned hash;
	int is_local_ip_any;
	
#ifdef EXTRA_DEBUG
	DBG("tcpconn_find: %d  port %d\n",id, port);
	if (ip) print_ip("tcpconn_find: ip ", ip, "\n");
#endif
	if (likely(id)){
		hash=tcp_id_hash(id);
		for (c=tcpconn_id_hash[hash]; c; c=c->id_next){
#ifdef EXTRA_DEBUG
			DBG("c=%p, c->id=%d, port=%d\n",c, c->id, c->rcv.src_port);
			print_ip("ip=", &c->rcv.src_ip, "\n");
#endif
			if ((id==c->id)&&(c->state!=S_CONN_BAD)) return c;
		}
	}else if (likely(ip)){
		hash=tcp_addr_hash(ip, port, l_ip, l_port);
		is_local_ip_any=ip_addr_any(l_ip);
		for (a=tcpconn_aliases_hash[hash]; a; a=a->next){
#ifdef EXTRA_DEBUG
			DBG("a=%p, c=%p, c->id=%d, alias port= %d port=%d\n", a, a->parent,
					a->parent->id, a->port, a->parent->rcv.src_port);
			print_ip("ip=",&a->parent->rcv.src_ip,"\n");
#endif
			if ( (a->parent->state!=S_CONN_BAD) && (port==a->port) &&
					((l_port==0) || (l_port==a->parent->rcv.dst_port)) &&
					(ip_addr_cmp(ip, &a->parent->rcv.src_ip)) &&
					(is_local_ip_any ||
						ip_addr_cmp(l_ip, &a->parent->rcv.dst_ip))
				)
				return a->parent;
		}
	}
	return 0;
}



/* _tcpconn_find with locks and timeout
 * local_addr contains the desired local ip:port. If null any local address 
 * will be used.  IN*ADDR_ANY or 0 port are wild cards.
 */
struct tcp_connection* tcpconn_get(int id, struct ip_addr* ip, int port,
									union sockaddr_union* local_addr,
									ticks_t timeout)
{
	struct tcp_connection* c;
	struct ip_addr local_ip;
	int local_port;
	
	local_port=0;
	if (likely(ip)){
		if (unlikely(local_addr)){
			su2ip_addr(&local_ip, local_addr);
			local_port=su_getport(local_addr);
		}else{
			ip_addr_mk_any(ip->af, &local_ip);
			local_port=0;
		}
	}
	TCPCONN_LOCK;
	c=_tcpconn_find(id, ip, port, &local_ip, local_port);
	if (likely(c)){ 
			atomic_inc(&c->refcnt);
			/* update the timeout only if the connection is not handled
			 * by a tcp reader (the tcp reader process uses c->timeout for 
			 * its own internal timeout and c->timeout will be overwritten
			 * anyway on return to tcp_main) */
			if (likely(c->reader_pid==0))
				c->timeout=get_ticks_raw()+timeout;
	}
	TCPCONN_UNLOCK;
	return c;
}



/* add c->dst:port, local_addr as an alias for the "id" connection, 
 * flags: TCP_ALIAS_FORCE_ADD  - add an alias even if a previous one exists
 *        TCP_ALIAS_REPLACE    - if a prev. alias exists, replace it with the
 *                                new one
 * returns 0 on success, <0 on failure ( -1  - null c, -2 too many aliases,
 *  -3 alias already present and pointing to another connection)
 * WARNING: must be called with TCPCONN_LOCK held */
inline static int _tcpconn_add_alias_unsafe(struct tcp_connection* c, int port,
										struct ip_addr* l_ip, int l_port,
										int flags)
{
	unsigned hash;
	struct tcp_conn_alias* a;
	struct tcp_conn_alias* nxt;
	struct tcp_connection* p;
	int is_local_ip_any;
	int i;
	int r;
	
	a=0;
	is_local_ip_any=ip_addr_any(l_ip);
	if (likely(c)){
		hash=tcp_addr_hash(&c->rcv.src_ip, port, l_ip, l_port);
		/* search the aliases for an already existing one */
		for (a=tcpconn_aliases_hash[hash], nxt=0; a; a=nxt){
			nxt=a->next;
			if ( (a->parent->state!=S_CONN_BAD) && (port==a->port) &&
					( (l_port==0) || (l_port==a->parent->rcv.dst_port)) &&
					(ip_addr_cmp(&c->rcv.src_ip, &a->parent->rcv.src_ip)) &&
					( is_local_ip_any || 
					  ip_addr_cmp(&a->parent->rcv.dst_ip, l_ip))
					){
				/* found */
				if (unlikely(a->parent!=c)){
					if (flags & TCP_ALIAS_FORCE_ADD)
						/* still have to walk the whole list to check if
						 * the alias was not already added */
						continue;
					else if (flags & TCP_ALIAS_REPLACE){
						/* remove the alias =>
						 * remove the current alias and all the following
						 *  ones from the corresponding connection, shift the 
						 *  connection aliases array and re-add the other 
						 *  aliases (!= current one) */
						p=a->parent;
						for (i=0; (i<p->aliases) && (&(p->con_aliases[i])!=a);
								i++);
						if (unlikely(i==p->aliases)){
							LOG(L_CRIT, "BUG: _tcpconn_add_alias_unsafe: "
									" alias %p not found in con %p (id %d)\n",
									a, p, p->id);
							goto error_not_found;
						}
						for (r=i; r<p->aliases; r++){
							tcpconn_listrm(
								tcpconn_aliases_hash[p->con_aliases[r].hash],
								&p->con_aliases[r], next, prev);
						}
						if (likely((i+1)<p->aliases)){
							memmove(&p->con_aliases[i], &p->con_aliases[i+1],
											(p->aliases-i-1)*
												sizeof(p->con_aliases[0]));
						}
						p->aliases--;
						/* re-add the remaining aliases */
						for (r=i; r<p->aliases; r++){
							tcpconn_listadd(
								tcpconn_aliases_hash[p->con_aliases[r].hash], 
								&p->con_aliases[r], next, prev);
						}
					}else
						goto error_sec;
				}else goto ok;
			}
		}
		if (unlikely(c->aliases>=TCP_CON_MAX_ALIASES)) goto error_aliases;
		c->con_aliases[c->aliases].parent=c;
		c->con_aliases[c->aliases].port=port;
		c->con_aliases[c->aliases].hash=hash;
		tcpconn_listadd(tcpconn_aliases_hash[hash], 
								&c->con_aliases[c->aliases], next, prev);
		c->aliases++;
	}else goto error_not_found;
ok:
#ifdef EXTRA_DEBUG
	if (a) DBG("_tcpconn_add_alias_unsafe: alias already present\n");
	else   DBG("_tcpconn_add_alias_unsafe: alias port %d for hash %d, id %d\n",
			port, hash, c->id);
#endif
	return 0;
error_aliases:
	/* too many aliases */
	return -2;
error_not_found:
	/* null connection */
	return -1;
error_sec:
	/* alias already present and pointing to a different connection
	 * (hijack attempt?) */
	return -3;
}



/* add port as an alias for the "id" connection, 
 * returns 0 on success,-1 on failure */
int tcpconn_add_alias(int id, int port, int proto)
{
	struct tcp_connection* c;
	int ret;
	struct ip_addr zero_ip;
	int r;
	
	/* fix the port */
	port=port?port:((proto==PROTO_TLS)?SIPS_PORT:SIP_PORT);
	TCPCONN_LOCK;
	/* check if alias already exists */
	c=_tcpconn_find(id, 0, 0, 0, 0);
	if (likely(c)){
		ip_addr_mk_any(c->rcv.src_ip.af, &zero_ip);
		
		/* alias src_ip:port, 0, 0 */
		ret=_tcpconn_add_alias_unsafe(c, port,  &zero_ip, 0, 
										tcp_alias_flags);
		if (ret<0 && ret!=-3) goto error;
		/* alias src_ip:port, local_ip, 0 */
		ret=_tcpconn_add_alias_unsafe(c, port,  &c->rcv.dst_ip, 0, 
										tcp_alias_flags);
		if (ret<0 && ret!=-3) goto error;
		/* alias src_ip:port, local_ip, local_port */
		ret=_tcpconn_add_alias_unsafe(c, port, &c->rcv.dst_ip, c->rcv.dst_port,
										tcp_alias_flags);
		if (unlikely(ret<0)) goto error;
	}else goto error_not_found;
	TCPCONN_UNLOCK;
	return 0;
error_not_found:
	TCPCONN_UNLOCK;
	LOG(L_ERR, "ERROR: tcpconn_add_alias: no connection found for id %d\n",id);
	return -1;
error:
	TCPCONN_UNLOCK;
	switch(ret){
		case -2:
			LOG(L_ERR, "ERROR: tcpconn_add_alias: too many aliases (%d)"
					" for connection %p (id %d) %s:%d <- %d\n",
					c->aliases, c, c->id, ip_addr2a(&c->rcv.src_ip),
					c->rcv.src_port, port);
			for (r=0; r<c->aliases; r++){
				LOG(L_ERR, "ERROR: tcpconn_add_alias: alias %d: for %p (%d)"
						" %s:%d <-%d hash %x\n",  r, c, c->id, 
						 ip_addr2a(&c->rcv.src_ip), c->rcv.src_port, 
						c->con_aliases[r].port, c->con_aliases[r].hash);
			}
			break;
		case -3:
			LOG(L_ERR, "ERROR: tcpconn_add_alias: possible port"
					" hijack attempt\n");
			LOG(L_ERR, "ERROR: tcpconn_add_alias: alias for %d port %d already"
						" present and points to another connection \n",
						c->id, port);
			break;
		default:
			LOG(L_ERR, "ERROR: tcpconn_add_alias: unkown error %d\n", ret);
	}
	return -1;
}



#ifdef TCP_FD_CACHE

static void tcp_fd_cache_init()
{
	int r;
	for (r=0; r<TCP_FD_CACHE_SIZE; r++)
		fd_cache[r].fd=-1;
}


inline static struct fd_cache_entry* tcp_fd_cache_get(struct tcp_connection *c)
{
	int h;
	
	h=c->id%TCP_FD_CACHE_SIZE;
	if ((fd_cache[h].fd>0) && (fd_cache[h].id==c->id) && (fd_cache[h].con==c))
		return &fd_cache[h];
	return 0;
}


inline static void tcp_fd_cache_rm(struct fd_cache_entry* e)
{
	e->fd=-1;
}


inline static void tcp_fd_cache_add(struct tcp_connection *c, int fd)
{
	int h;
	
	h=c->id%TCP_FD_CACHE_SIZE;
	if (likely(fd_cache[h].fd>0))
		close(fd_cache[h].fd);
	fd_cache[h].fd=fd;
	fd_cache[h].id=c->id;
	fd_cache[h].con=c;
}

#endif /* TCP_FD_CACHE */



inline static int tcpconn_chld_put(struct tcp_connection* tcpconn);



/* finds a tcpconn & sends on it
 * uses the dst members to, proto (TCP|TLS) and id and tries to send
 *  from the "from" address (if non null and id==0)
 * returns: number of bytes written (>=0) on success
 *          <0 on error */
int tcp_send(struct dest_info* dst, union sockaddr_union* from,
					char* buf, unsigned len)
{
	struct tcp_connection *c;
	struct tcp_connection *tmp;
	struct ip_addr ip;
	int port;
	int fd;
	long response[2];
	int n;
	int do_close_fd;
#ifdef TCP_BUF_WRITE
	int enable_write_watch;
#endif /* TCP_BUF_WRITE */
#ifdef TCP_FD_CACHE
	struct fd_cache_entry* fd_cache_e;
	int use_fd_cache;
	
	use_fd_cache=tcp_options.fd_cache;
	fd_cache_e=0;
#endif /* TCP_FD_CACHE */
	do_close_fd=1; /* close the fd on exit */
	port=su_getport(&dst->to);
	if (likely(port)){
		su2ip_addr(&ip, &dst->to);
		c=tcpconn_get(dst->id, &ip, port, from, tcp_con_lifetime); 
	}else if (likely(dst->id)){
		c=tcpconn_get(dst->id, 0, 0, 0, tcp_con_lifetime);
	}else{
		LOG(L_CRIT, "BUG: tcp_send called with null id & to\n");
		return -1;
	}
	
	if (likely(dst->id)){
		if (unlikely(c==0)) {
			if (likely(port)){
				/* try again w/o id */
				c=tcpconn_get(0, &ip, port, from, tcp_con_lifetime);
				goto no_id;
			}else{
				LOG(L_ERR, "ERROR: tcp_send: id %d not found, dropping\n",
						dst->id);
				return -1;
			}
		}else goto get_fd;
	}
no_id:
		if (unlikely(c==0)){
			DBG("tcp_send: no open tcp connection found, opening new one\n");
			/* create tcp connection */
			if (likely(from==0)){
				/* check to see if we have to use a specific source addr. */
				switch (dst->to.s.sa_family) {
					case AF_INET:
							from = tcp_source_ipv4;
						break;
#ifdef USE_IPV6
					case AF_INET6:
							from = tcp_source_ipv6;
						break;
#endif
					default:
						/* error, bad af, ignore ... */
						break;
				}
			}
#if defined(TCP_CONNECT_WAIT) && defined(TCP_BUF_WRITE)
			if (likely(tcp_options.tcp_connect_wait && 
						tcp_options.tcp_buf_write )){
				/* FIXME: flag for timer? + check again */
				if (unlikely(*tcp_connections_no >= tcp_max_connections)){
					LOG(L_ERR, "ERROR: tcp_send: maximum number of connections"
								" exceeded (%d/%d)\n",
								*tcp_connections_no, tcp_max_connections);
					return -1;
				}
				c=tcpconn_new(-1, &dst->to, from, 0, dst->proto,
								S_CONN_PENDING);
				if (unlikely(c==0)){
					LOG(L_ERR, "ERROR: tcp_send: could not create new"
							" connection\n");
					return -1;
				}
				c->flags|=F_CONN_PENDING|F_CONN_FD_CLOSED;
				atomic_set(&c->refcnt, 2); /* ref from here and from main hash
											 table */
				/* add it to id hash and aliases */
				if (unlikely(tcpconn_add(c)==0)){
					LOG(L_ERR, "ERROR: tcp_send: could not add "
									"connection %p\n", c);
					_tcpconn_free(c);
					n=-1;
					goto end_no_conn;
				}
				/* do connect and if src ip or port changed, update the 
				 * aliases */
				if (unlikely((fd=tcpconn_finish_connect(c, from))<0)){
					LOG(L_ERR, "ERROR: tcp_send: tcpconn_finish_connect(%p)"
							" failed\n", c);
					goto conn_wait_error;
				}
				/* ? TODO: it might be faster just to queue the write directly
				 *  and send to main CONN_NEW_PENDING_WRITE */
				/* delay sending the fd to main after the send */
				
				/* NOTE: no lock here, because the connection is marked as
				 * pending and nobody else will try to write on it. However
				 * this might produce out-of-order writes. If this is not
				 * desired either lock before the write or use 
				 * _wbufq_insert(...) */
				n=_tcpconn_write_nb(fd, c, buf, len);
				if (unlikely(n<(int)len)){
					if ((n>=0) || errno==EAGAIN || errno==EWOULDBLOCK){
						DBG("tcp_send: pending write on new connection %p "
								" (%d/%d bytes written)\n", c, n, len);
						if (n<0) n=0;
						/* add to the write queue */
						lock_get(&c->write_lock);
							if (unlikely(_wbufq_insert(c, buf+n, len-n)<0)){
								lock_release(&c->write_lock);
								n=-1;
								LOG(L_ERR, "ERROR: tcp_send: EAGAIN and"
										" write queue full or failed for %p\n",
										c);
								goto conn_wait_error;
							}
						lock_release(&c->write_lock);
						/* send to tcp_main */
						response[0]=(long)c;
						response[1]=CONN_NEW_PENDING_WRITE;
						if (unlikely(send_fd(unix_tcp_sock, response, 
												sizeof(response), fd) <= 0)){
							LOG(L_ERR, "BUG: tcp_send: CONN_NEW_PENDING_WRITE"
										" for %p failed:" " %s (%d)\n",
										c, strerror(errno), errno);
							goto conn_wait_error;
						}
						n=len;
						goto end;
					}
					/* error: destroy it directly */
					LOG(L_ERR, "ERROR: tcp_send: connect & send "
										" for %p failed:" " %s (%d)\n",
										c, strerror(errno), errno);
					goto conn_wait_error;
				}
				LOG(L_INFO, "tcp_send: quick connect for %p\n", c);
				/* send to tcp_main */
				response[0]=(long)c;
				response[1]=CONN_NEW_COMPLETE;
				if (unlikely(send_fd(unix_tcp_sock, response, 
										sizeof(response), fd) <= 0)){
					LOG(L_ERR, "BUG: tcp_send: CONN_NEW_COMPLETE  for %p"
								" failed:" " %s (%d)\n",
								c, strerror(errno), errno);
					goto conn_wait_error;
				}
				goto end;
			}
#endif /* TCP_CONNECT_WAIT  && TCP_BUF_WRITE */
			if (unlikely((c=tcpconn_connect(&dst->to, from, dst->proto))==0)){
				LOG(L_ERR, "ERROR: tcp_send: connect failed\n");
				return -1;
			}
			atomic_set(&c->refcnt, 2); /* ref. from here and it will also
			                              be added in the tcp_main hash */
			fd=c->s;
			c->flags|=F_CONN_FD_CLOSED; /* not yet opened in main */
			/* ? TODO: it might be faster just to queue the write and
			 * send to main a CONN_NEW_PENDING_WRITE */
			
			/* send the new tcpconn to "tcp main" */
			response[0]=(long)c;
			response[1]=CONN_NEW;
			n=send_fd(unix_tcp_sock, response, sizeof(response), c->s);
			if (unlikely(n<=0)){
				LOG(L_ERR, "BUG: tcp_send: failed send_fd: %s (%d)\n",
						strerror(errno), errno);
				/* we can safely delete it, it's not referenced by anybody */
				_tcpconn_free(c);
				n=-1;
				goto end_no_conn;
			}
			goto send_it;
		}
get_fd:
#ifdef TCP_BUF_WRITE
		/* if data is already queued, we don't need the fd any more */
		if (unlikely(tcp_options.tcp_buf_write && (_wbufq_non_empty(c)
#ifdef TCP_CONNECT_WAIT
												|| (c->state==S_CONN_PENDING)
#endif /* TCP_CONNECT_WAIT */
						) )){
			lock_get(&c->write_lock);
				if (likely(_wbufq_non_empty(c)
#ifdef TCP_CONNECT_WAIT
							|| (c->state==S_CONN_PENDING)
#endif /* TCP_CONNECT_WAIT */

							)){
					do_close_fd=0;
					if (unlikely(_wbufq_add(c, buf, len)<0)){
						lock_release(&c->write_lock);
						n=-1;
						goto error;
					}
					n=len;
					lock_release(&c->write_lock);
					goto release_c;
				}
			lock_release(&c->write_lock);
		}
#endif /* TCP_BUF_WRITE */
		/* check if this is not the same reader process holding
		 *  c  and if so send directly on c->fd */
		if (c->reader_pid==my_pid()){
			DBG("tcp_send: send from reader (%d (%d)), reusing fd\n",
					my_pid(), process_no);
			fd=c->fd;
			do_close_fd=0; /* don't close the fd on exit, it's in use */
#ifdef TCP_FD_CACHE
			use_fd_cache=0; /* don't cache: problems would arise due to the
							   close() on cache eviction (if the fd is still 
							   used). If it has to be cached then dup() _must_ 
							   be used */
		}else if (likely(use_fd_cache && 
							((fd_cache_e=tcp_fd_cache_get(c))!=0))){
			fd=fd_cache_e->fd;
			do_close_fd=0;
			DBG("tcp_send: found fd in cache ( %d, %p, %d)\n",
					fd, c, fd_cache_e->id);
#endif /* TCP_FD_CACHE */
		}else{
			DBG("tcp_send: tcp connection found (%p), acquiring fd\n", c);
			/* get the fd */
			response[0]=(long)c;
			response[1]=CONN_GET_FD;
			n=send_all(unix_tcp_sock, response, sizeof(response));
			if (unlikely(n<=0)){
				LOG(L_ERR, "BUG: tcp_send: failed to get fd(write):%s (%d)\n",
						strerror(errno), errno);
				n=-1;
				goto release_c;
			}
			DBG("tcp_send, c= %p, n=%d\n", c, n);
			n=receive_fd(unix_tcp_sock, &tmp, sizeof(tmp), &fd, MSG_WAITALL);
			if (unlikely(n<=0)){
				LOG(L_ERR, "BUG: tcp_send: failed to get fd(receive_fd):"
							" %s (%d)\n", strerror(errno), errno);
				n=-1;
				do_close_fd=0;
				goto release_c;
			}
			if (unlikely(c!=tmp)){
				LOG(L_CRIT, "BUG: tcp_send: get_fd: got different connection:"
						"  %p (id= %d, refcnt=%d state=%d) != "
						"  %p (n=%d)\n",
						  c,   c->id,   atomic_get(&c->refcnt),   c->state,
						  tmp, n
				   );
				n=-1; /* fail */
				goto end;
			}
			DBG("tcp_send: after receive_fd: c= %p n=%d fd=%d\n",c, n, fd);
		}
	
	
send_it:
	DBG("tcp_send: sending...\n");
	lock_get(&c->write_lock);
#ifdef TCP_BUF_WRITE
	if (likely(tcp_options.tcp_buf_write)){
		if (_wbufq_non_empty(c)
#ifdef TCP_CONNECT_WAIT
			|| (c->state==S_CONN_PENDING) 
#endif /* TCP_CONNECT_WAIT */
			){
			if (unlikely(_wbufq_add(c, buf, len)<0)){
				lock_release(&c->write_lock);
				n=-1;
				goto error;
			}
			lock_release(&c->write_lock);
			n=len;
			goto end;
		}
		n=_tcpconn_write_nb(fd, c, buf, len);
	}else{
#endif /* TCP_BUF_WRITE */
#ifdef USE_TLS
	if (c->type==PROTO_TLS)
		n=tls_blocking_write(c, fd, buf, len);
	else
#endif
		/* n=tcp_blocking_write(c, fd, buf, len); */
		n=tsend_stream(fd, buf, len, tcp_send_timeout*1000); 
#ifdef TCP_BUF_WRITE
	}
#else /* ! TCP_BUF_WRITE */
	lock_release(&c->write_lock);
#endif /* TCP_BUF_WRITE */
	
	DBG("tcp_send: after real write: c= %p n=%d fd=%d\n",c, n, fd);
	DBG("tcp_send: buf=\n%.*s\n", (int)len, buf);
	if (unlikely(n<(int)len)){
#ifdef TCP_BUF_WRITE
		if (tcp_options.tcp_buf_write && 
				((n>=0) || errno==EAGAIN || errno==EWOULDBLOCK)){
			enable_write_watch=_wbufq_empty(c);
			if (n<0) n=0;
			if (unlikely(_wbufq_add(c, buf+n, len-n)<0)){
				lock_release(&c->write_lock);
				n=-1;
				goto error;
			}
			lock_release(&c->write_lock);
			n=len;
			if (likely(enable_write_watch)){
				response[0]=(long)c;
				response[1]=CONN_QUEUED_WRITE;
				if (send_all(unix_tcp_sock, response, sizeof(response)) <= 0){
					LOG(L_ERR, "BUG: tcp_send: error return failed "
								"(write):%s (%d)\n", strerror(errno), errno);
					n=-1;
					goto error;
				}
			}
			goto end;
		}else{
			lock_release(&c->write_lock);
		}
#endif /* TCP_BUF_WRITE */
		LOG(L_ERR, "ERROR: tcp_send: failed to send on %p: %s (%d)\n",
					c, strerror(errno), errno);
#ifdef TCP_BUF_WRITE
error:
#endif /* TCP_BUF_WRITE */
		/* error on the connection , mark it as bad and set 0 timeout */
		c->state=S_CONN_BAD;
		c->timeout=get_ticks_raw();
		/* tell "main" it should drop this (optional it will t/o anyway?)*/
		response[0]=(long)c;
		response[1]=CONN_ERROR;
		if (send_all(unix_tcp_sock, response, sizeof(response))<=0){
			LOG(L_CRIT, "BUG: tcp_send: error return failed (write):%s (%d)\n",
					strerror(errno), errno);
			tcpconn_chld_put(c); /* deref. it manually */
			n=-1;
		}
		/* CONN_ERROR will auto-dec refcnt => we must not call tcpconn_put 
		 * if it succeeds */
#ifdef TCP_FD_CACHE
		if (unlikely(fd_cache_e)){
			LOG(L_ERR, "ERROR: tcp_send: error on cached fd, removing from the"
					"cache (%d, %p, %d)\n", 
					fd, fd_cache_e->con, fd_cache_e->id);
			tcp_fd_cache_rm(fd_cache_e);
			close(fd);
		}else
#endif /* TCP_FD_CACHE */
		if (do_close_fd) close(fd);
		return n; /* error return, no tcpconn_put */
	}
	
#ifdef TCP_BUF_WRITE
	lock_release(&c->write_lock);
	if (likely(tcp_options.tcp_buf_write)){
		if (unlikely(c->state==S_CONN_CONNECT))
			c->state=S_CONN_OK;
	}
#endif /* TCP_BUF_WRITE */
end:
#ifdef TCP_FD_CACHE
	if (unlikely((fd_cache_e==0) && use_fd_cache)){
		tcp_fd_cache_add(c, fd);
	}else
#endif /* TCP_FD_CACHE */
	if (do_close_fd) close(fd);
release_c:
	tcpconn_chld_put(c); /* release c (dec refcnt & free on 0) */
end_no_conn:
	return n;
#ifdef TCP_CONNECT_WAIT
conn_wait_error:
	/* connect or send failed on newly created connection which was not
	 * yet sent to tcp_main (but was already hashed) => don't send to main,
	 * unhash and destroy directly (if refcnt>2 it will be destroyed when the 
	 * last sender releases the connection (tcpconn_chld_put(c))) or when
	 * tcp_main receives a CONN_ERROR it*/
	c->state=S_CONN_BAD;
	TCPCONN_LOCK;
		if (c->flags & F_CONN_HASHED){
			/* if some other parallel tcp_send did send CONN_ERROR to
			 * tcp_main, the connection might be already detached */
			_tcpconn_detach(c);
			c->flags&=~F_CONN_HASHED;
			TCPCONN_UNLOCK;
			tcpconn_put(c);
		}else
			TCPCONN_UNLOCK;
	/* dec refcnt -> mark it for destruction */
	tcpconn_chld_put(c);
	return -1;
#endif /* TCP_CONNET_WAIT */
}



int tcp_init(struct socket_info* sock_info)
{
	union sockaddr_union* addr;
	int optval;
#ifdef HAVE_TCP_ACCEPT_FILTER
	struct accept_filter_arg afa;
#endif /* HAVE_TCP_ACCEPT_FILTER */
#ifdef DISABLE_NAGLE
	int flag;
	struct protoent* pe;

	if (tcp_proto_no==-1){ /* if not already set */
		pe=getprotobyname("tcp");
		if (pe==0){
			LOG(L_ERR, "ERROR: tcp_init: could not get TCP protocol number\n");
			tcp_proto_no=-1;
		}else{
			tcp_proto_no=pe->p_proto;
		}
	}
#endif
	
	addr=&sock_info->su;
	/* sock_info->proto=PROTO_TCP; */
	if (init_su(addr, &sock_info->address, sock_info->port_no)<0){
		LOG(L_ERR, "ERROR: tcp_init: could no init sockaddr_union\n");
		goto error;
	}
	sock_info->socket=socket(AF2PF(addr->s.sa_family), SOCK_STREAM, 0);
	if (sock_info->socket==-1){
		LOG(L_ERR, "ERROR: tcp_init: socket: %s\n", strerror(errno));
		goto error;
	}
#ifdef DISABLE_NAGLE
	flag=1;
	if ( (tcp_proto_no!=-1) &&
		 (setsockopt(sock_info->socket, tcp_proto_no , TCP_NODELAY,
					 &flag, sizeof(flag))<0) ){
		LOG(L_ERR, "ERROR: tcp_init: could not disable Nagle: %s\n",
				strerror(errno));
	}
#endif


#if  !defined(TCP_DONT_REUSEADDR) 
	/* Stevens, "Network Programming", Section 7.5, "Generic Socket
     * Options": "...server started,..a child continues..on existing
	 * connection..listening server is restarted...call to bind fails
	 * ... ALL TCP servers should specify the SO_REUSEADDRE option 
	 * to allow the server to be restarted in this situation
	 *
	 * Indeed, without this option, the server can't restart.
	 *   -jiri
	 */
	optval=1;
	if (setsockopt(sock_info->socket, SOL_SOCKET, SO_REUSEADDR,
				(void*)&optval, sizeof(optval))==-1) {
		LOG(L_ERR, "ERROR: tcp_init: setsockopt %s\n",
			strerror(errno));
		goto error;
	}
#endif
	/* tos */
	optval = tos;
	if (setsockopt(sock_info->socket, IPPROTO_IP, IP_TOS, (void*)&optval, 
				sizeof(optval)) ==-1){
		LOG(L_WARN, "WARNING: tcp_init: setsockopt tos: %s\n", strerror(errno));
		/* continue since this is not critical */
	}
#ifdef HAVE_TCP_DEFER_ACCEPT
	/* linux only */
	if (tcp_options.defer_accept){
		optval=tcp_options.defer_accept;
		if (setsockopt(sock_info->socket, IPPROTO_TCP, TCP_DEFER_ACCEPT,
					(void*)&optval, sizeof(optval)) ==-1){
			LOG(L_WARN, "WARNING: tcp_init: setsockopt TCP_DEFER_ACCEPT %s\n",
						strerror(errno));
		/* continue since this is not critical */
		}
	}
#endif /* HAVE_TCP_DEFFER_ACCEPT */
#ifdef HAVE_TCP_SYNCNT
	if (tcp_options.syncnt){
		optval=tcp_options.syncnt;
		if (setsockopt(sock_info->socket, IPPROTO_TCP, TCP_SYNCNT, &optval,
						sizeof(optval))<0){
			LOG(L_WARN, "WARNING: tcp_init: failed to set"
						" maximum SYN retr. count: %s\n", strerror(errno));
		}
	}
#endif
#ifdef HAVE_TCP_LINGER2
	if (tcp_options.linger2){
		optval=tcp_options.linger2;
		if (setsockopt(sock_info->socket, IPPROTO_TCP, TCP_LINGER2, &optval,
						sizeof(optval))<0){
			LOG(L_WARN, "WARNING: tcp_init: failed to set"
						" maximum LINGER2 timeout: %s\n", strerror(errno));
		}
	}
#endif
	init_sock_keepalive(sock_info->socket);
	if (bind(sock_info->socket, &addr->s, sockaddru_len(*addr))==-1){
		LOG(L_ERR, "ERROR: tcp_init: bind(%x, %p, %d) on %s:%d : %s\n",
				sock_info->socket,  &addr->s, 
				(unsigned)sockaddru_len(*addr),
				sock_info->address_str.s,
				sock_info->port_no,
				strerror(errno));
		goto error;
	}
	if (listen(sock_info->socket, TCP_LISTEN_BACKLOG)==-1){
		LOG(L_ERR, "ERROR: tcp_init: listen(%x, %p, %d) on %s: %s\n",
				sock_info->socket, &addr->s, 
				(unsigned)sockaddru_len(*addr),
				sock_info->address_str.s,
				strerror(errno));
		goto error;
	}
#ifdef HAVE_TCP_ACCEPT_FILTER
	/* freebsd */
	if (tcp_options.defer_accept){
		memset(&afa, 0, sizeof(afa));
		strcpy(afa.af_name, "dataready");
		if (setsockopt(sock_info->socket, SOL_SOCKET, SO_ACCEPTFILTER,
					(void*)&afa, sizeof(afa)) ==-1){
			LOG(L_WARN, "WARNING: tcp_init: setsockopt SO_ACCEPTFILTER %s\n",
						strerror(errno));
		/* continue since this is not critical */
		}
	}
#endif /* HAVE_TCP_ACCEPT_FILTER */
	
	return 0;
error:
	if (sock_info->socket!=-1){
		close(sock_info->socket);
		sock_info->socket=-1;
	}
	return -1;
}



/* close tcp_main's fd from a tcpconn
 * WARNING: call only in tcp_main context */
inline static void tcpconn_close_main_fd(struct tcp_connection* tcpconn)
{
	int fd;
	
	
	fd=tcpconn->s;
#ifdef USE_TLS
	/*FIXME: lock ->writelock ? */
	if (tcpconn->type==PROTO_TLS)
		tls_close(tcpconn, fd);
#endif
#ifdef TCP_FD_CACHE
	if (likely(tcp_options.fd_cache)) shutdown(fd, SHUT_RDWR);
#endif /* TCP_FD_CACHE */
close_again:
	if (unlikely(close(fd)<0)){
		if (errno==EINTR)
			goto close_again;
		LOG(L_ERR, "ERROR: tcpconn_put_destroy; close() failed: %s (%d)\n",
				strerror(errno), errno);
	}
}



/* dec refcnt & frees the connection if refcnt==0
 * returns 1 if the connection is freed, 0 otherwise
 *
 * WARNING: use only from child processes */
inline static int tcpconn_chld_put(struct tcp_connection* tcpconn)
{
	if (unlikely(atomic_dec_and_test(&tcpconn->refcnt))){
		DBG("tcpconn_chld_put: destroying connection %p (%d, %d) "
				"flags %04x\n", tcpconn, tcpconn->id,
				tcpconn->s, tcpconn->flags);
		/* sanity checks */
		membar_read_atomic_op(); /* make sure we see the current flags */
		if (unlikely(!(tcpconn->flags & F_CONN_FD_CLOSED) || 
					 !(tcpconn->flags & F_CONN_REMOVED) || 
					(tcpconn->flags & 
					 	(F_CONN_HASHED|F_CONN_MAIN_TIMER| F_CONN_WRITE_W)) )){
			LOG(L_CRIT, "BUG: tcpconn_chld_put: %p bad flags = %0x\n",
					tcpconn, tcpconn->flags);
			abort();
		}
		_tcpconn_free(tcpconn); /* destroys also the wbuf_q if still present*/
		return 1;
	}
	return 0;
}



/* simple destroy function (the connection should be already removed
 * from the hashes and the fds should not be watched anymore for IO)
 */
inline static void tcpconn_destroy(struct tcp_connection* tcpconn)
{
		DBG("tcpconn_destroy: destroying connection %p (%d, %d) "
				"flags %04x\n", tcpconn, tcpconn->id,
				tcpconn->s, tcpconn->flags);
		if (unlikely(tcpconn->flags & F_CONN_HASHED)){
			LOG(L_CRIT, "BUG: tcpconn_destroy: called with hashed"
						" connection (%p)\n", tcpconn);
			/* try to continue */
			if (likely(tcpconn->flags & F_CONN_MAIN_TIMER))
				local_timer_del(&tcp_main_ltimer, &tcpconn->timer);
			TCPCONN_LOCK;
				_tcpconn_detach(tcpconn);
			TCPCONN_UNLOCK;
		}
		if (likely(!(tcpconn->flags & F_CONN_FD_CLOSED))){
			tcpconn_close_main_fd(tcpconn);
			(*tcp_connections_no)--;
		}
		_tcpconn_free(tcpconn); /* destroys also the wbuf_q if still present*/
}



/* tries to destroy the connection: dec. refcnt and if 0 destroys the
 *  connection, else it will mark it as BAD and close the main fds
 *
 * returns 1 if the connection was destroyed, 0 otherwise
 *
 * WARNING: - the connection _has_ to be removed from the hash and timer
 *  first (use tcpconn_try_unhash() for this )
 *         - the fd should not be watched anymore (io_watch_del()...)
 *         - must be called _only_ from the tcp_main process context
 *          (or else the fd will remain open)
 */
inline static int tcpconn_put_destroy(struct tcp_connection* tcpconn)
{
	if (unlikely((tcpconn->flags & 
						(F_CONN_WRITE_W|F_CONN_HASHED|F_CONN_MAIN_TIMER)) ||
				!(tcpconn->flags & F_CONN_REMOVED) )){
		/* sanity check */
		if (unlikely(tcpconn->flags & F_CONN_HASHED)){
			LOG(L_CRIT, "BUG: tcpconn_destroy: called with hashed and/or"
						"on timer connection (%p), flags = %0x\n",
						tcpconn, tcpconn->flags);
			/* try to continue */
			if (likely(tcpconn->flags & F_CONN_MAIN_TIMER))
				local_timer_del(&tcp_main_ltimer, &tcpconn->timer);
			TCPCONN_LOCK;
				_tcpconn_detach(tcpconn);
			TCPCONN_UNLOCK;
		}else{
			LOG(L_CRIT, "BUG: tcpconn_put_destroy: %p flags = %0x\n",
					tcpconn, tcpconn->flags);
		}
	}
	tcpconn->state=S_CONN_BAD;
	/* in case it's still in a reader timer */
	tcpconn->timeout=get_ticks_raw();
	/* fast close: close fds now */
	if (likely(!(tcpconn->flags & F_CONN_FD_CLOSED))){
		tcpconn_close_main_fd(tcpconn);
		tcpconn->flags|=F_CONN_FD_CLOSED;
		(*tcp_connections_no)--;
	}
	/* all the flags / ops on the tcpconn must be done prior to decrementing
	 * the refcnt. and at least a membar_write_atomic_op() mem. barrier or
	 *  a mb_atomic_* op must * be used to make sure all the changed flags are
	 *  written into memory prior to the new refcnt value */
	if (unlikely(mb_atomic_dec_and_test(&tcpconn->refcnt))){
		_tcpconn_free(tcpconn);
		return 1;
	}
	return 0;
}



/* try to remove a connection from the hashes and timer.
 * returns 1 if the connection was removed, 0 if not (connection not in
 *  hash)
 *
 * WARNING: call it only in the  tcp_main process context or else the
 *  timer removal won't work.
 */
inline static int tcpconn_try_unhash(struct tcp_connection* tcpconn)
{
	if (likely(tcpconn->flags & F_CONN_HASHED)){
		tcpconn->state=S_CONN_BAD;
		if (likely(tcpconn->flags & F_CONN_MAIN_TIMER)){
			local_timer_del(&tcp_main_ltimer, &tcpconn->timer);
			tcpconn->flags&=~F_CONN_MAIN_TIMER;
		}else
			/* in case it's still in a reader timer */
			tcpconn->timeout=get_ticks_raw();
		TCPCONN_LOCK;
			if (tcpconn->flags & F_CONN_HASHED){
				tcpconn->flags&=~F_CONN_HASHED;
				_tcpconn_detach(tcpconn);
				TCPCONN_UNLOCK;
			}else{
				/* tcp_send was faster and did unhash it itself */
				TCPCONN_UNLOCK;
				return 0;
			}
#ifdef TCP_BUF_WRITE
		/* empty possible write buffers (optional) */
		if (unlikely(_wbufq_non_empty(tcpconn))){
			lock_get(&tcpconn->write_lock);
				/* check again, while holding the lock */
				if (likely(_wbufq_non_empty(tcpconn)))
					_wbufq_destroy(&tcpconn->wbuf_q);
			lock_release(&tcpconn->write_lock);
		}
#endif /* TCP_BUF_WRITE */
		return 1;
	}
	return 0;
}



#ifdef SEND_FD_QUEUE
struct send_fd_info{
	struct tcp_connection* tcp_conn;
	ticks_t expire;
	int unix_sock;
	unsigned int retries; /* debugging */
};

struct tcp_send_fd_q{
	struct send_fd_info* data; /* buffer */
	struct send_fd_info* crt;  /* pointer inside the buffer */
	struct send_fd_info* end;  /* points after the last valid position */
};


static struct tcp_send_fd_q send2child_q;



static int send_fd_queue_init(struct tcp_send_fd_q *q, unsigned int size)
{
	q->data=pkg_malloc(size*sizeof(struct send_fd_info));
	if (q->data==0){
		LOG(L_ERR, "ERROR: send_fd_queue_init: out of memory\n");
		return -1;
	}
	q->crt=&q->data[0];
	q->end=&q->data[size];
	return 0;
}

static void send_fd_queue_destroy(struct tcp_send_fd_q *q)
{
	if (q->data){
		pkg_free(q->data);
		q->data=0;
		q->crt=q->end=0;
	}
}



static int init_send_fd_queues()
{
	if (send_fd_queue_init(&send2child_q, SEND_FD_QUEUE_SIZE)!=0)
		goto error;
	return 0;
error:
	LOG(L_ERR, "ERROR: init_send_fd_queues: init failed\n");
	return -1;
}



static void destroy_send_fd_queues()
{
	send_fd_queue_destroy(&send2child_q);
}




inline static int send_fd_queue_add(	struct tcp_send_fd_q* q, 
										int unix_sock,
										struct tcp_connection *t)
{
	struct send_fd_info* tmp;
	unsigned long new_size;
	
	if (q->crt>=q->end){
		new_size=q->end-&q->data[0];
		if (new_size< MAX_SEND_FD_QUEUE_SIZE/2){
			new_size*=2;
		}else new_size=MAX_SEND_FD_QUEUE_SIZE;
		if (unlikely(q->crt>=&q->data[new_size])){
			LOG(L_ERR, "ERROR: send_fd_queue_add: queue full: %ld/%ld\n",
					(long)(q->crt-&q->data[0]-1), new_size);
			goto error;
		}
		LOG(L_CRIT, "INFO: send_fd_queue: queue full: %ld, extending to %ld\n",
				(long)(q->end-&q->data[0]), new_size);
		tmp=pkg_realloc(q->data, new_size*sizeof(struct send_fd_info));
		if (unlikely(tmp==0)){
			LOG(L_ERR, "ERROR: send_fd_queue_add: out of memory\n");
			goto error;
		}
		q->crt=(q->crt-&q->data[0])+tmp;
		q->data=tmp;
		q->end=&q->data[new_size];
	}
	q->crt->tcp_conn=t;
	q->crt->unix_sock=unix_sock;
	q->crt->expire=get_ticks_raw()+SEND_FD_QUEUE_TIMEOUT;
	q->crt->retries=0;
	q->crt++;
	return 0;
error:
	return -1;
}



inline static void send_fd_queue_run(struct tcp_send_fd_q* q)
{
	struct send_fd_info* p;
	struct send_fd_info* t;
	
	for (p=t=&q->data[0]; p<q->crt; p++){
		if (unlikely(send_fd(p->unix_sock, &(p->tcp_conn),
					sizeof(struct tcp_connection*), p->tcp_conn->s)<=0)){
			if ( ((errno==EAGAIN)||(errno==EWOULDBLOCK)) && 
							((s_ticks_t)(p->expire-get_ticks_raw())>0)){
				/* leave in queue for a future try */
				*t=*p;
				t->retries++;
				t++;
			}else{
				LOG(L_ERR, "ERROR: run_send_fd_queue: send_fd failed"
						   " on socket %d , queue entry %ld, retries %d,"
						   " connection %p, tcp socket %d, errno=%d (%s) \n",
						   p->unix_sock, (long)(p-&q->data[0]), p->retries,
						   p->tcp_conn, p->tcp_conn->s, errno,
						   strerror(errno));
#ifdef TCP_BUF_WRITE
				if (p->tcp_conn->flags & F_CONN_WRITE_W){
					io_watch_del(&io_h, p->tcp_conn->s, -1, IO_FD_CLOSING);
					p->tcp_conn->flags &=~F_CONN_WRITE_W;
				}
#endif
				p->tcp_conn->flags &= ~F_CONN_READER;
				if (likely(tcpconn_try_unhash(p->tcp_conn)))
					tcpconn_put(p->tcp_conn);
				tcpconn_put_destroy(p->tcp_conn); /* dec refcnt & destroy */
			}
		}
	}
	q->crt=t;
}
#else
#define send_fd_queue_run(q)
#endif


/* non blocking write() on a tcpconnection, unsafe version (should be called
 * while holding  c->write_lock). The fd should be non-blocking.
 *  returns number of bytes written on success, -1 on error (and sets errno)
 */
inline static int _tcpconn_write_nb(int fd, struct tcp_connection* c,
									char* buf, int len)
{
	int n;
	
again:
#ifdef USE_TLS
	if (unlikely(c->type==PROTO_TLS))
		/* FIXME: tls_nonblocking_write !! */
		n=tls_blocking_write(c, fd, buf, len);
	else
#endif /* USE_TLS */
		n=send(fd, buf, len,
#ifdef HAVE_MSG_NOSIGNAL
					MSG_NOSIGNAL
#else
					0
#endif /* HAVE_MSG_NOSIGNAL */
			  );
	if (unlikely(n<0)){
		if (errno==EINTR) goto again;
	}
	return n;
}



/* handles io from a tcp child process
 * params: tcp_c - pointer in the tcp_children array, to the entry for
 *                 which an io event was detected 
 *         fd_i  - fd index in the fd_array (usefull for optimizing
 *                 io_watch_deletes)
 * returns:  handle_* return convention: -1 on error, 0 on EAGAIN (no more
 *           io events queued), >0 on success. success/error refer only to
 *           the reads from the fd.
 */
inline static int handle_tcp_child(struct tcp_child* tcp_c, int fd_i)
{
	struct tcp_connection* tcpconn;
	long response[2];
	int cmd;
	int bytes;
	int n;
	ticks_t t;
	ticks_t crt_timeout;
	
	if (unlikely(tcp_c->unix_sock<=0)){
		/* (we can't have a fd==0, 0 is never closed )*/
		LOG(L_CRIT, "BUG: handle_tcp_child: fd %d for %d "
				"(pid %d, ser no %d)\n", tcp_c->unix_sock,
				(int)(tcp_c-&tcp_children[0]), tcp_c->pid, tcp_c->proc_no);
		goto error;
	}
	/* read until sizeof(response)
	 * (this is a SOCK_STREAM so read is not atomic) */
	bytes=recv_all(tcp_c->unix_sock, response, sizeof(response), MSG_DONTWAIT);
	if (unlikely(bytes<(int)sizeof(response))){
		if (bytes==0){
			/* EOF -> bad, child has died */
			DBG("DBG: handle_tcp_child: dead tcp child %d (pid %d, no %d)"
					" (shutting down?)\n", (int)(tcp_c-&tcp_children[0]), 
					tcp_c->pid, tcp_c->proc_no );
			/* don't listen on it any more */
			io_watch_del(&io_h, tcp_c->unix_sock, fd_i, 0); 
			goto error; /* eof. so no more io here, it's ok to return error */
		}else if (bytes<0){
			/* EAGAIN is ok if we try to empty the buffer
			 * e.g.: SIGIO_RT overflow mode or EPOLL ET */
			if ((errno!=EAGAIN) && (errno!=EWOULDBLOCK)){
				LOG(L_CRIT, "ERROR: handle_tcp_child: read from tcp child %ld "
						" (pid %d, no %d) %s [%d]\n",
						(long)(tcp_c-&tcp_children[0]), tcp_c->pid,
						tcp_c->proc_no, strerror(errno), errno );
			}else{
				bytes=0;
			}
			/* try to ignore ? */
			goto end;
		}else{
			/* should never happen */
			LOG(L_CRIT, "BUG: handle_tcp_child: too few bytes received (%d)\n",
					bytes );
			bytes=0; /* something was read so there is no error; otoh if
					  receive_fd returned less then requested => the receive
					  buffer is empty => no more io queued on this fd */
			goto end;
		}
	}
	
	DBG("handle_tcp_child: reader response= %lx, %ld from %d \n",
					response[0], response[1], (int)(tcp_c-&tcp_children[0]));
	cmd=response[1];
	tcpconn=(struct tcp_connection*)response[0];
	if (unlikely(tcpconn==0)){
		/* should never happen */
		LOG(L_CRIT, "BUG: handle_tcp_child: null tcpconn pointer received"
				 " from tcp child %d (pid %d): %lx, %lx\n",
				 	(int)(tcp_c-&tcp_children[0]), tcp_c->pid,
					response[0], response[1]) ;
		goto end;
	}
	switch(cmd){
		case CONN_RELEASE:
			tcp_c->busy--;
			if (unlikely(tcpconn_put(tcpconn))){
				tcpconn_destroy(tcpconn);
				break;
			}
			if (unlikely(tcpconn->state==S_CONN_BAD)){ 
#ifdef TCP_BUF_WRITE
				if (unlikely(tcpconn->flags & F_CONN_WRITE_W)){
					io_watch_del(&io_h, tcpconn->s, -1, IO_FD_CLOSING);
					tcpconn->flags &= ~F_CONN_WRITE_W;
				}
#endif /* TCP_BUF_WRITE */
				if (tcpconn_try_unhash(tcpconn))
					tcpconn_put_destroy(tcpconn);
				break;
			}
			/* update the timeout*/
			t=get_ticks_raw();
			tcpconn->timeout=t+tcp_con_lifetime;
			crt_timeout=tcp_con_lifetime;
#ifdef TCP_BUF_WRITE
			if (unlikely(tcp_options.tcp_buf_write && 
							_wbufq_non_empty(tcpconn) )){
				if (unlikely(TICKS_LE(t, tcpconn->wbuf_q.wr_timeout))){
					DBG("handle_tcp_child: wr. timeout on CONN_RELEASE for %p "
							"refcnt= %d\n", tcpconn,
							atomic_get(&tcpconn->refcnt));
					/* timeout */
					if (unlikely(tcpconn->flags & F_CONN_WRITE_W)){
						io_watch_del(&io_h, tcpconn->s, -1, IO_FD_CLOSING);
						tcpconn->flags&=~F_CONN_WRITE_W;
					}
					if (tcpconn_try_unhash(tcpconn))
						tcpconn_put_destroy(tcpconn);
					break;
				}else{
					crt_timeout=MIN_unsigned(tcp_con_lifetime,
											tcpconn->wbuf_q.wr_timeout-t);
				}
			}
#endif /* TCP_BUF_WRITE */
			/* re-activate the timer */
			tcpconn->timer.f=tcpconn_main_timeout;
			local_timer_reinit(&tcpconn->timer);
			local_timer_add(&tcp_main_ltimer, &tcpconn->timer, crt_timeout, t);
			/* must be after the de-ref*/
			tcpconn->flags|=F_CONN_MAIN_TIMER;
			tcpconn->flags&=~(F_CONN_REMOVED|F_CONN_READER);
#ifdef TCP_BUF_WRITE
			if (unlikely(tcpconn->flags & F_CONN_WRITE_W))
				n=io_watch_chg(&io_h, tcpconn->s, POLLIN| POLLOUT, -1);
			else
#endif /* TCP_BUF_WRITE */
				n=io_watch_add(&io_h, tcpconn->s, POLLIN, F_TCPCONN, tcpconn);
			if (unlikely(n<0)){
				LOG(L_CRIT, "ERROR: tcp_main: handle_tcp_child: failed to add"
						" new socket to the fd list\n");
				tcpconn->flags|=F_CONN_REMOVED;
#ifdef TCP_BUF_WRITE
				if (unlikely(tcpconn->flags & F_CONN_WRITE_W)){
					io_watch_del(&io_h, tcpconn->s, -1, IO_FD_CLOSING);
					tcpconn->flags&=~F_CONN_WRITE_W;
				}
#endif /* TCP_BUF_WRITE */
				if (tcpconn_try_unhash(tcpconn));
					tcpconn_put_destroy(tcpconn);
				break;
			}
			DBG("handle_tcp_child: CONN_RELEASE  %p refcnt= %d\n", 
							tcpconn, atomic_get(&tcpconn->refcnt));
			break;
		case CONN_ERROR:
		case CONN_DESTROY:
		case CONN_EOF:
			/* WARNING: this will auto-dec. refcnt! */
				tcp_c->busy--;
				/* main doesn't listen on it => we don't have to delete it
				 if (tcpconn->s!=-1)
					io_watch_del(&io_h, tcpconn->s, -1, IO_FD_CLOSING);
				*/
#ifdef TCP_BUF_WRITE
				if ((tcpconn->flags & F_CONN_WRITE_W) && (tcpconn->s!=-1)){
					io_watch_del(&io_h, tcpconn->s, -1, IO_FD_CLOSING);
					tcpconn->flags&=~F_CONN_WRITE_W;
				}
#endif /* TCP_BUF_WRITE */
				if (tcpconn_try_unhash(tcpconn))
					tcpconn_put(tcpconn);
				tcpconn_put_destroy(tcpconn); /* deref & delete if refcnt==0 */
				break;
		default:
				LOG(L_CRIT, "BUG: handle_tcp_child:  unknown cmd %d"
									" from tcp reader %d\n",
									cmd, (int)(tcp_c-&tcp_children[0]));
	}
end:
	return bytes;
error:
	return -1;
}



/* handles io from a "generic" ser process (get fd or new_fd from a tcp_send)
 * 
 * params: p     - pointer in the ser processes array (pt[]), to the entry for
 *                 which an io event was detected
 *         fd_i  - fd index in the fd_array (usefull for optimizing
 *                 io_watch_deletes)
 * returns:  handle_* return convention:
 *          -1 on error reading from the fd,
 *           0 on EAGAIN  or when no  more io events are queued 
 *             (receive buffer empty),
 *           >0 on successfull reads from the fd (the receive buffer might
 *             be non-empty).
 */
inline static int handle_ser_child(struct process_table* p, int fd_i)
{
	struct tcp_connection* tcpconn;
	long response[2];
	int cmd;
	int bytes;
	int ret;
	int fd;
	int flags;
	ticks_t t;
	
	ret=-1;
	if (unlikely(p->unix_sock<=0)){
		/* (we can't have a fd==0, 0 is never closed )*/
		LOG(L_CRIT, "BUG: handle_ser_child: fd %d for %d "
				"(pid %d)\n", p->unix_sock, (int)(p-&pt[0]), p->pid);
		goto error;
	}
			
	/* get all bytes and the fd (if transmitted)
	 * (this is a SOCK_STREAM so read is not atomic) */
	bytes=receive_fd(p->unix_sock, response, sizeof(response), &fd,
						MSG_DONTWAIT);
	if (unlikely(bytes<(int)sizeof(response))){
		/* too few bytes read */
		if (bytes==0){
			/* EOF -> bad, child has died */
			DBG("DBG: handle_ser_child: dead child %d, pid %d"
					" (shutting down?)\n", (int)(p-&pt[0]), p->pid);
			/* don't listen on it any more */
			io_watch_del(&io_h, p->unix_sock, fd_i, 0);
			goto error; /* child dead => no further io events from it */
		}else if (bytes<0){
			/* EAGAIN is ok if we try to empty the buffer
			 * e.g: SIGIO_RT overflow mode or EPOLL ET */
			if ((errno!=EAGAIN) && (errno!=EWOULDBLOCK)){
				LOG(L_CRIT, "ERROR: handle_ser_child: read from child %d  "
						"(pid %d):  %s [%d]\n", (int)(p-&pt[0]), p->pid,
						strerror(errno), errno);
				ret=-1;
			}else{
				ret=0;
			}
			/* try to ignore ? */
			goto end;
		}else{
			/* should never happen */
			LOG(L_CRIT, "BUG: handle_ser_child: too few bytes received (%d)\n",
					bytes );
			ret=0; /* something was read so there is no error; otoh if
					  receive_fd returned less then requested => the receive
					  buffer is empty => no more io queued on this fd */
			goto end;
		}
	}
	ret=1; /* something was received, there might be more queued */
	DBG("handle_ser_child: read response= %lx, %ld, fd %d from %d (%d)\n",
					response[0], response[1], fd, (int)(p-&pt[0]), p->pid);
	cmd=response[1];
	tcpconn=(struct tcp_connection*)response[0];
	if (unlikely(tcpconn==0)){
		LOG(L_CRIT, "BUG: handle_ser_child: null tcpconn pointer received"
				 " from child %d (pid %d): %lx, %lx\n",
				 	(int)(p-&pt[0]), p->pid, response[0], response[1]) ;
		goto end;
	}
	switch(cmd){
		case CONN_ERROR:
			LOG(L_ERR, "handle_ser_child: ERROR: received CON_ERROR for %p"
					" (id %d), refcnt %d\n", 
					tcpconn, tcpconn->id, atomic_get(&tcpconn->refcnt));
#ifdef TCP_CONNECT_WAIT
			/* if the connection is pending => it might be on the way of
			 * reaching tcp_main (e.g. CONN_NEW_COMPLETE or 
			 *  CONN_NEW_PENDING_WRITE) =>  it cannot be destroyed here */
			if ( !(tcpconn->flags & F_CONN_PENDING) && 
					tcpconn_try_unhash(tcpconn) )
				tcpconn_put(tcpconn);
#else /* ! TCP_CONNECT_WAIT */
			if ( tcpconn_try_unhash(tcpconn) )
				tcpconn_put(tcpconn);
#endif /* TCP_CONNECT_WAIT */
			if ( (!(tcpconn->flags & F_CONN_REMOVED) ||
					(tcpconn->flags & F_CONN_WRITE_W) ) && (tcpconn->s!=-1)){
				io_watch_del(&io_h, tcpconn->s, -1, IO_FD_CLOSING);
				tcpconn->flags|=F_CONN_REMOVED;
				tcpconn->flags&=~F_CONN_WRITE_W;
			}
			tcpconn_put_destroy(tcpconn); /* dec refcnt & destroy on 0 */
			break;
		case CONN_GET_FD:
			/* send the requested FD  */
			/* WARNING: take care of setting refcnt properly to
			 * avoid race conditions */
			if (unlikely(send_fd(p->unix_sock, &tcpconn, sizeof(tcpconn),
								tcpconn->s)<=0)){
				LOG(L_ERR, "ERROR: handle_ser_child: send_fd failed\n");
			}
			break;
		case CONN_NEW:
			/* update the fd in the requested tcpconn*/
			/* WARNING: take care of setting refcnt properly to
			 * avoid race conditions */
			if (unlikely(fd==-1)){
				LOG(L_CRIT, "BUG: handle_ser_child: CONN_NEW:"
							" no fd received\n");
				tcpconn->flags|=F_CONN_FD_CLOSED;
				tcpconn_put_destroy(tcpconn);
				break;
			}
			(*tcp_connections_no)++;
			tcpconn->s=fd;
			/* add tcpconn to the list*/
			tcpconn_add(tcpconn);
			/* update the timeout*/
			t=get_ticks_raw();
			tcpconn->timeout=t+tcp_con_lifetime;
			/* activate the timer (already properly init. in tcpconn_new())
			 * no need for reinit */
			local_timer_add(&tcp_main_ltimer, &tcpconn->timer, 
								tcp_con_lifetime, t);
			tcpconn->flags|=F_CONN_MAIN_TIMER;
			tcpconn->flags&=~(F_CONN_REMOVED|F_CONN_FD_CLOSED);
			flags=POLLIN 
#ifdef TCP_BUF_WRITE
					/* not used for now, the connection is sent to tcp_main
					 * before knowing if we can write on it or we should 
					 * wait */
					| (((int)!(tcpconn->flags & F_CONN_WRITE_W)-1) & POLLOUT)
#endif /* TCP_BUF_WRITE */
					;
			if (unlikely(
					io_watch_add(&io_h, tcpconn->s, flags,
												F_TCPCONN, tcpconn)<0)){
				LOG(L_CRIT, "ERROR: tcp_main: handle_ser_child: failed to add"
						" new socket to the fd list\n");
				tcpconn->flags|=F_CONN_REMOVED;
				tcpconn->flags&=~F_CONN_WRITE_W;
				tcpconn_try_unhash(tcpconn); /*  unhash & dec refcnt */
				tcpconn_put_destroy(tcpconn);
			}
			break;
#ifdef TCP_BUF_WRITE
		case CONN_QUEUED_WRITE:
			/* received only if the wr. queue is empty and a write finishes
			 * with EAGAIN (common after connect())
			 * it should only enable write watching on the fd. The connection
			 * should be  already in the hash. The refcnt is not changed.
			 */
			if (unlikely((tcpconn->state==S_CONN_BAD) || 
							!(tcpconn->flags & F_CONN_HASHED) ))
				break;
			if (!(tcpconn->flags & F_CONN_WRITE_W)){
				t=get_ticks_raw();
				if (likely((tcpconn->flags & F_CONN_MAIN_TIMER) && 
					(TICKS_LT(tcpconn->wbuf_q.wr_timeout, tcpconn->timeout)) &&
						TICKS_LT(t, tcpconn->wbuf_q.wr_timeout) )){
					/* _wbufq_nonempty() is guaranteed here */
					/* update the timer */
					local_timer_del(&tcp_main_ltimer, &tcpconn->timer);
					local_timer_reinit(&tcpconn->timer);
					local_timer_add(&tcp_main_ltimer, &tcpconn->timer,
										tcpconn->wbuf_q.wr_timeout-t, t);
					DBG("tcp_main: handle_ser_child: CONN_QUEUED_WRITE; %p "
							"timeout adjusted to %d s\n", tcpconn, 
							TICKS_TO_S(tcpconn->wbuf_q.wr_timeout-t));
				}
				if (tcpconn->flags& F_CONN_REMOVED){
					if (unlikely(io_watch_add(&io_h, tcpconn->s, POLLOUT,
												F_TCPCONN, tcpconn)<0)){
						LOG(L_CRIT, "ERROR: tcp_main: handle_ser_child: failed"
								    " to enable write watch on socket\n");
						if (tcpconn_try_unhash(tcpconn))
							tcpconn_put_destroy(tcpconn);
						break;
					}
				}else{
					if (unlikely(io_watch_chg(&io_h, tcpconn->s,
												POLLIN|POLLOUT, -1)<0)){
						LOG(L_CRIT, "ERROR: tcp_main: handle_ser_child: failed"
								    " to change socket watch events\n");
						io_watch_del(&io_h, tcpconn->s, -1, IO_FD_CLOSING);
						tcpconn->flags|=F_CONN_REMOVED;
						if (tcpconn_try_unhash(tcpconn))
							tcpconn_put_destroy(tcpconn);
						break;
					}
				}
				tcpconn->flags|=F_CONN_WRITE_W;
			}else{
				LOG(L_WARN, "tcp_main: hanlder_ser_child: connection %p"
							" already watched for write\n", tcpconn);
			}
			break;
#ifdef TCP_CONNECT_WAIT
		case CONN_NEW_COMPLETE:
		case CONN_NEW_PENDING_WRITE:
				/* received when a pending connect completes in the same
				 * tcp_send() that initiated it
				 * the connection is already in the hash with S_CONN_PENDING
				 * state (added by tcp_send()) and refcnt at least 1 (for the
				 *  hash)*/
			tcpconn->flags&=~(F_CONN_PENDING|F_CONN_FD_CLOSED|F_CONN_REMOVED);
			if (unlikely((tcpconn->state==S_CONN_BAD) || (fd==-1))){
				if (unlikely(fd==-1))
					LOG(L_CRIT, "BUG: handle_ser_child: CONN_NEW_COMPLETE:"
								" no fd received\n");
				else
					LOG(L_WARN, "WARNING: handle_ser_child: CONN_NEW_COMPLETE:"
							" received connection with error\n");
				tcpconn->flags|=F_CONN_FD_CLOSED;
				tcpconn->state=S_CONN_BAD;
				tcpconn_try_unhash(tcpconn);
				tcpconn_put_destroy(tcpconn);
				break;
			}
			tcpconn->state=S_CONN_OK;
			(*tcp_connections_no)++;
			tcpconn->s=fd;
			/* update the timeout*/
			t=get_ticks_raw();
			tcpconn->timeout=t+tcp_con_lifetime;
			/* activate the timer (already properly init. in tcpconn_new())
			 * no need for reinit */
			local_timer_add(&tcp_main_ltimer, &tcpconn->timer, 
								tcp_con_lifetime, t);
			tcpconn->flags|=F_CONN_MAIN_TIMER;
			if (unlikely(cmd==CONN_NEW_COMPLETE)){
				/* check if needs to be watched for write */
				lock_get(&tcpconn->write_lock);
					/* if queue non empty watch it for write */
					flags=(_wbufq_empty(tcpconn)-1)&POLLOUT;
				lock_release(&tcpconn->write_lock);
				tcpconn->flags|=(!(flags&POLLOUT)-1)&F_CONN_WRITE_W;
			}else{
				/* CONN_NEW_PENDING_WRITE */
				/* no need to check, we have something queued for write */
				flags=POLLOUT;
				tcpconn->flags|=F_CONN_WRITE_W;
			}
			flags|=POLLIN;
			if (unlikely(
					io_watch_add(&io_h, tcpconn->s, flags,
												F_TCPCONN, tcpconn)<0)){
				LOG(L_CRIT, "ERROR: tcp_main: handle_ser_child: failed to add"
						" new socket to the fd list\n");
				tcpconn->flags|=F_CONN_REMOVED;
				tcpconn->flags&=~F_CONN_WRITE_W;
				tcpconn_try_unhash(tcpconn); /*  unhash & dec refcnt */
				tcpconn_put_destroy(tcpconn);
			}
			break;
#endif /* TCP_CONNECT_WAIT */
#endif /* TCP_BUF_WRITE */
		default:
			LOG(L_CRIT, "BUG: handle_ser_child: unknown cmd %d\n", cmd);
	}
end:
	return ret;
error:
	return -1;
}



/* sends a tcpconn + fd to a choosen child */
inline static int send2child(struct tcp_connection* tcpconn)
{
	int i;
	int min_busy;
	int idx;
	static int crt=0; /* current child */
	int last;
	
	min_busy=tcp_children[0].busy;
	idx=0;
	last=crt+tcp_children_no;
	for (; crt<last; crt++){
		i=crt%tcp_children_no;
		if (!tcp_children[i].busy){
			idx=i;
			min_busy=0;
			break;
		}else if (min_busy>tcp_children[i].busy){
			min_busy=tcp_children[i].busy;
			idx=i;
		}
	}
	crt=idx+1; /* next time we start with crt%tcp_children_no */
	
	tcp_children[idx].busy++;
	tcp_children[idx].n_reqs++;
	if (unlikely(min_busy)){
		DBG("WARNING: send2child: no free tcp receiver, "
				" connection passed to the least busy one (%d)\n",
				min_busy);
	}
	DBG("send2child: to tcp child %d %d(%d), %p\n", idx, 
					tcp_children[idx].proc_no,
					tcp_children[idx].pid, tcpconn);
	/* first make sure this child doesn't have pending request for
	 * tcp_main (to avoid a possible deadlock: e.g. child wants to
	 * send a release command, but the master fills its socket buffer
	 * with new connection commands => deadlock) */
	/* answer tcp_send requests first */
	while(handle_ser_child(&pt[tcp_children[idx].proc_no], -1)>0);
	/* process tcp readers requests */
	while(handle_tcp_child(&tcp_children[idx], -1)>0);
		
#ifdef SEND_FD_QUEUE
	/* if queue full, try to queue the io */
	if (unlikely(send_fd(tcp_children[idx].unix_sock, &tcpconn,
							sizeof(tcpconn), tcpconn->s)<=0)){
		if ((errno==EAGAIN)||(errno==EWOULDBLOCK)){
			/* FIXME: remove after debugging */
			 LOG(L_CRIT, "INFO: tcp child %d, socket %d: queue full,"
					 	" %d requests queued (total handled %d)\n",
					idx, tcp_children[idx].unix_sock, min_busy,
					tcp_children[idx].n_reqs-1);
			if (send_fd_queue_add(&send2child_q, tcp_children[idx].unix_sock, 
						tcpconn)!=0){
				LOG(L_ERR, "ERROR: send2child: queue send op. failed\n");
				return -1;
			}
		}else{
			LOG(L_ERR, "ERROR: send2child: send_fd failed\n");
			return -1;
		}
	}
#else
	if (unlikely(send_fd(tcp_children[idx].unix_sock, &tcpconn,
						sizeof(tcpconn), tcpconn->s)<=0)){
		LOG(L_ERR, "ERROR: send2child: send_fd failed\n");
		return -1;
	}
#endif
	
	return 0;
}



/* handles a new connection, called internally by tcp_main_loop/handle_io.
 * params: si - pointer to one of the tcp socket_info structures on which
 *              an io event was detected (connection attempt)
 * returns:  handle_* return convention: -1 on error, 0 on EAGAIN (no more
 *           io events queued), >0 on success. success/error refer only to
 *           the accept.
 */
static inline int handle_new_connect(struct socket_info* si)
{
	union sockaddr_union su;
	union sockaddr_union sock_name;
	unsigned sock_name_len;
	union sockaddr_union* dst_su;
	struct tcp_connection* tcpconn;
	socklen_t su_len;
	int new_sock;
	
	/* got a connection on r */
	su_len=sizeof(su);
	new_sock=accept(si->socket, &(su.s), &su_len);
	if (unlikely(new_sock==-1)){
		if ((errno==EAGAIN)||(errno==EWOULDBLOCK))
			return 0;
		LOG(L_ERR,  "WARNING: handle_new_connect: error while accepting"
				" connection(%d): %s\n", errno, strerror(errno));
		return -1;
	}
	if (unlikely(*tcp_connections_no>=tcp_max_connections)){
		LOG(L_ERR, "ERROR: maximum number of connections exceeded: %d/%d\n",
					*tcp_connections_no, tcp_max_connections);
		close(new_sock);
		return 1; /* success, because the accept was succesfull */
	}
	if (unlikely(init_sock_opt_accept(new_sock)<0)){
		LOG(L_ERR, "ERROR: handle_new_connect: init_sock_opt failed\n");
		close(new_sock);
		return 1; /* success, because the accept was succesfull */
	}
	(*tcp_connections_no)++;
	
	dst_su=&si->su;
	if (unlikely(si->flags & SI_IS_ANY)){
		/* INADDR_ANY => get local dst */
		sock_name_len=sizeof(sock_name);
		if (getsockname(new_sock, &sock_name.s, &sock_name_len)!=0){
			LOG(L_ERR, "ERROR: handle_new_connect:"
						" getsockname failed: %s(%d)\n",
						strerror(errno), errno);
			/* go on with the 0.0.0.0 dst from the sock_info */
		}else{
			dst_su=&sock_name;
		}
	}
	/* add socket to list */
	tcpconn=tcpconn_new(new_sock, &su, dst_su, si, si->proto, S_CONN_ACCEPT);
	if (likely(tcpconn)){
#ifdef TCP_PASS_NEW_CONNECTION_ON_DATA
		atomic_set(&tcpconn->refcnt, 1); /* safe, not yet available to the
											outside world */
		tcpconn_add(tcpconn);
		/* activate the timer */
		local_timer_add(&tcp_main_ltimer, &tcpconn->timer, 
								tcp_con_lifetime, get_ticks_raw());
		tcpconn->flags|=F_CONN_MAIN_TIMER;
		tcpconn->flags&=~F_CONN_REMOVED;
		if (unlikely(io_watch_add(&io_h, tcpconn->s, POLLIN, 
													F_TCPCONN, tcpconn)<0)){
			LOG(L_CRIT, "ERROR: tcp_main: handle_new_connect: failed to add"
						" new socket to the fd list\n");
			tcpconn->flags|=F_CONN_REMOVED;
			if (tcpconn_try_unhash(tcpconn))
				tcpconn_put_destroy(tcpconn);
		}
#else
		atomic_set(&tcpconn->refcnt, 2); /* safe, not yet available to the
											outside world */
		/* prepare it for passing to a child */
		tcpconn->flags|=F_CONN_READER;
		tcpconn_add(tcpconn);
		DBG("handle_new_connect: new connection: %p %d flags: %04x\n",
			tcpconn, tcpconn->s, tcpconn->flags);
		if(unlikely(send2child(tcpconn)<0)){
			LOG(L_ERR,"ERROR: handle_new_connect: no children "
					"available\n");
			tcpconn->flags&=~F_CONN_READER;
			tcpconn_put(tcpconn);
			tcpconn_try_unhash(tcpconn);
			tcpconn_put_destroy(tcpconn);
		}
#endif
	}else{ /*tcpconn==0 */
		LOG(L_ERR, "ERROR: handle_new_connect: tcpconn_new failed, "
				"closing socket\n");
		close(new_sock);
		(*tcp_connections_no)--;
	}
	return 1; /* accept() was succesfull */
}



/* handles an io event on one of the watched tcp connections
 * 
 * params: tcpconn - pointer to the tcp_connection for which we have an io ev.
 *         fd_i    - index in the fd_array table (needed for delete)
 * returns:  handle_* return convention, but on success it always returns 0
 *           (because it's one-shot, after a succesful execution the fd is
 *            removed from tcp_main's watch fd list and passed to a child =>
 *            tcp_main is not interested in further io events that might be
 *            queued for this fd)
 */
inline static int handle_tcpconn_ev(struct tcp_connection* tcpconn, short ev, 
										int fd_i)
{
#ifdef TCP_BUF_WRITE
	int empty_q;
#endif /* TCP_BUF_WRITE */
	/*  is refcnt!=0 really necessary? 
	 *  No, in fact it's a bug: I can have the following situation: a send only
	 *   tcp connection used by n processes simultaneously => refcnt = n. In 
	 *   the same time I can have a read event and this situation is perfectly
	 *   valid. -- andrei
	 */
#if 0
	if ((tcpconn->refcnt!=0)){
		/* FIXME: might be valid for sigio_rt iff fd flags are not cleared
		 *        (there is a short window in which it could generate a sig
		 *         that would be catched by tcp_main) */
		LOG(L_CRIT, "BUG: handle_tcpconn_ev: io event on referenced"
					" tcpconn (%p), refcnt=%d, fd=%d\n",
					tcpconn, tcpconn->refcnt, tcpconn->s);
		return -1;
	}
#endif
	/* pass it to child, so remove it from the io watch list  and the local
	 *  timer */
	DBG("handle_tcpconn_ev: ev (%0x) on %p %d\n", ev, tcpconn, tcpconn->s);
#ifdef TCP_BUF_WRITE
	if (unlikely((ev & POLLOUT) && (tcpconn->flags & F_CONN_WRITE_W))){
		if (unlikely(wbufq_run(tcpconn->s, tcpconn, &empty_q)<0)){
			io_watch_del(&io_h, tcpconn->s, fd_i, 0);
			tcpconn->flags|=F_CONN_REMOVED;
			tcpconn->flags&=~F_CONN_WRITE_W;
			if (unlikely(!tcpconn_try_unhash(tcpconn))){
				LOG(L_CRIT, "BUG: tcpconn_ev: unhashed connection %p\n",
							tcpconn);
			}
			tcpconn_put_destroy(tcpconn);
			goto error;
		}
		if (empty_q){
			if (tcpconn->flags & F_CONN_REMOVED){
				if (unlikely(io_watch_del(&io_h, tcpconn->s, fd_i, 0)==-1))
					goto error;
			}else{
				if (unlikely(io_watch_chg(&io_h, tcpconn->s,
											POLLIN, fd_i)==-1))
					goto error;
			}
			tcpconn->flags&=~F_CONN_WRITE_W;
		}
		ev&=~POLLOUT; /* clear POLLOUT */
	}
	if (likely(ev && !(tcpconn->flags & F_CONN_REMOVED))){
		/* if still some other IO event (POLLIN|POLLHUP|POLLERR) and
		 * connection is still watched in tcp_main for reads, send it to a
		 * child and stop watching it for input (but continue watching for
		 *  writes if needed): */
		if (unlikely(tcpconn->flags & F_CONN_WRITE_W)){
			if (unlikely(io_watch_chg(&io_h, tcpconn->s, POLLOUT, fd_i)==-1))
				goto error;
		}else
#else
	{
#endif /* TCP_BUF_WRITE */
			if (unlikely(io_watch_del(&io_h, tcpconn->s, fd_i, 0)==-1))
				goto error;
		DBG("tcp: DBG: sendig to child, events %x\n", ev);
		tcpconn->flags|=F_CONN_REMOVED|F_CONN_READER;
		local_timer_del(&tcp_main_ltimer, &tcpconn->timer);
		tcpconn->flags&=~F_CONN_MAIN_TIMER;
		tcpconn_ref(tcpconn); /* refcnt ++ */
		if (unlikely(send2child(tcpconn)<0)){
			LOG(L_ERR,"ERROR: handle_tcpconn_ev: no children available\n");
			tcpconn->flags&=~F_CONN_READER;
#ifdef TCP_BUF_WRITE
			if (tcpconn->flags & F_CONN_WRITE_W){
				io_watch_del(&io_h, tcpconn->s, fd_i, 0);
				tcpconn->flags&=~F_CONN_WRITE_W;
			}
#endif /* TCP_BUF_WRITE */
			tcpconn_put(tcpconn);
			tcpconn_try_unhash(tcpconn); 
			tcpconn_put_destroy(tcpconn); /* because of the tcpconn_ref() */
		}
	}
	return 0; /* we are not interested in possibly queued io events, 
				 the fd was either passed to a child, closed, or for writes,
				 everything possible was already written */
error:
	return -1;
}



/* generic handle io routine, it will call the appropiate
 *  handle_xxx() based on the fd_map type
 *
 * params:  fm  - pointer to a fd hash entry
 *          idx - index in the fd_array (or -1 if not known)
 * return: -1 on error
 *          0 on EAGAIN or when by some other way it is known that no more 
 *            io events are queued on the fd (the receive buffer is empty).
 *            Usefull to detect when there are no more io events queued for
 *            sigio_rt, epoll_et, kqueue.
 *         >0 on successfull read from the fd (when there might be more io
 *            queued -- the receive buffer might still be non-empty)
 */
inline static int handle_io(struct fd_map* fm, short ev, int idx)
{	
	int ret;

	/* update the local config */
	cfg_update();
	
	switch(fm->type){
		case F_SOCKINFO:
			ret=handle_new_connect((struct socket_info*)fm->data);
			break;
		case F_TCPCONN:
			ret=handle_tcpconn_ev((struct tcp_connection*)fm->data, ev, idx);
			break;
		case F_TCPCHILD:
			ret=handle_tcp_child((struct tcp_child*)fm->data, idx);
			break;
		case F_PROC:
			ret=handle_ser_child((struct process_table*)fm->data, idx);
			break;
		case F_NONE:
			LOG(L_CRIT, "BUG: handle_io: empty fd map: %p {%d, %d, %p},"
						" idx %d\n", fm, fm->fd, fm->type, fm->data, idx);
			goto error;
		default:
			LOG(L_CRIT, "BUG: handle_io: uknown fd type %d\n", fm->type); 
			goto error;
	}
	return ret;
error:
	return -1;
}



/* timer handler for tcpconnection handled by tcp_main */
static ticks_t tcpconn_main_timeout(ticks_t t, struct timer_ln* tl, void* data)
{
	struct tcp_connection *c;
	int fd;
	
	c=(struct tcp_connection*)data; 
	/* or (struct tcp...*)(tl-offset(c->timer)) */
	
#ifdef TCP_BUF_WRITE
	DBG( "tcp_main: entering timer for %p (ticks=%d, timeout=%d (%d s), "
			"wr_timeout=%d (%d s)), write queue: %d bytes\n",
			c, t, c->timeout, TICKS_TO_S(c->timeout-t),
			c->wbuf_q.wr_timeout, TICKS_TO_S(c->wbuf_q.wr_timeout-t),
			c->wbuf_q.queued);
	
	if (TICKS_LT(t, c->timeout) && 
			(!tcp_options.tcp_buf_write | _wbufq_empty(c) |
				TICKS_LT(t, c->wbuf_q.wr_timeout)) ){
		if (unlikely(tcp_options.tcp_buf_write && _wbufq_non_empty(c)))
			return (ticks_t)MIN_unsigned(c->timeout-t, c->wbuf_q.wr_timeout-t);
		else
			return (ticks_t)(c->timeout - t);
	}
#else /* ! TCP_BUF_WRITE */
	if (TICKS_LT(t, c->timeout)){
		/* timeout extended, exit */
		return (ticks_t)(c->timeout - t);
	}
#endif /* TCP_BUF_WRITE */
	DBG("tcp_main: timeout for %p\n", c);
	if (likely(c->flags & F_CONN_HASHED)){
		c->flags&=~(F_CONN_HASHED|F_CONN_MAIN_TIMER);
		c->state=S_CONN_BAD;
		TCPCONN_LOCK;
			_tcpconn_detach(c);
		TCPCONN_UNLOCK;
	}else{
		c->flags&=~F_CONN_MAIN_TIMER;
		LOG(L_CRIT, "BUG: tcp_main: timer: called with unhashed connection %p"
				"\n", c);
		tcpconn_ref(c); /* ugly hack to try to go on */
	}
	fd=c->s;
	if (likely(fd>0)){
		if (likely(!(c->flags & F_CONN_REMOVED)
#ifdef TCP_BUF_WRITE
			|| (c->flags & F_CONN_WRITE_W)
#endif /* TCP_BUF_WRITE */
			)){
			io_watch_del(&io_h, fd, -1, IO_FD_CLOSING);
			c->flags|=F_CONN_REMOVED;
#ifdef TCP_BUF_WRITE
			c->flags&=~F_CONN_WRITE_W;
#endif /* TCP_BUF_WRITE */
		}
	}
	tcpconn_put_destroy(c);
	return 0;
}



static inline void tcp_timer_run()
{
	ticks_t ticks;
	
	ticks=get_ticks_raw();
	if (unlikely((ticks-tcp_main_prev_ticks)<TCPCONN_TIMEOUT_MIN_RUN)) return;
	tcp_main_prev_ticks=ticks;
	local_timer_run(&tcp_main_ltimer, ticks);
}



/* keep in sync with tcpconn_destroy, the "delete" part should be
 * the same except for io_watch_del..
 * Note: this function is called only on shutdown by the main ser process via
 * cleanup(). However it's also safe to call it from the tcp_main process.
 * => with the ser shutdown exception, it cannot execute in parallel
 * with tcpconn_add() or tcpconn_destroy()*/
static inline void tcpconn_destroy_all()
{
	struct tcp_connection *c, *next;
	unsigned h;
	int fd;
	
	
	TCPCONN_LOCK; 
	for(h=0; h<TCP_ID_HASH_SIZE; h++){
		c=tcpconn_id_hash[h];
		while(c){
			next=c->id_next;
				if (is_tcp_main){
					/* we cannot close or remove the fd if we are not in the
					 * tcp main proc.*/
					if ((c->flags & F_CONN_MAIN_TIMER)){
						local_timer_del(&tcp_main_ltimer, &c->timer);
						c->flags&=~F_CONN_MAIN_TIMER;
					} /* else still in some reader */
					fd=c->s;
					if (fd>0 && (!(c->flags & F_CONN_REMOVED)
#ifdef TCP_BUF_WRITE
								|| (c->flags & F_CONN_WRITE_W)
#endif /* TCP_BUF_WRITE */
								)){
						io_watch_del(&io_h, fd, -1, IO_FD_CLOSING);
						c->flags|=F_CONN_REMOVED;
#ifdef TCP_BUF_WRITE
						c->flags&=~F_CONN_WRITE_W;
#endif /* TCP_BUF_WRITE */
					}
				}else{
					fd=-1;
				}
#ifdef USE_TLS
				if (fd>0 && c->type==PROTO_TLS)
					tls_close(c, fd);
#endif
				_tcpconn_rm(c);
				if (fd>0) {
#ifdef TCP_FD_CACHE
					if (likely(tcp_options.fd_cache)) shutdown(fd, SHUT_RDWR);
#endif /* TCP_FD_CACHE */
					close(fd);
				}
				(*tcp_connections_no)--;
			c=next;
		}
	}
	TCPCONN_UNLOCK;
}



/* tcp main loop */
void tcp_main_loop()
{

	struct socket_info* si;
	int r;
	
	is_tcp_main=1; /* mark this process as tcp main */
	
	tcp_main_max_fd_no=get_max_open_fds();
	/* init send fd queues (here because we want mem. alloc only in the tcp
	 *  process */
#ifdef SEND_FD_QUEUE
	if (init_send_fd_queues()<0){
		LOG(L_CRIT, "ERROR: init_tcp: could not init send fd queues\n");
		goto error;
	}
#endif
	/* init io_wait (here because we want the memory allocated only in
	 * the tcp_main process) */
	if  (init_io_wait(&io_h, tcp_main_max_fd_no, tcp_poll_method)<0)
		goto error;
	/* init: start watching all the fds*/
	
	/* init local timer */
	tcp_main_prev_ticks=get_ticks_raw();
	if (init_local_timer(&tcp_main_ltimer, get_ticks_raw())!=0){
		LOG(L_ERR, "ERROR: init_tcp: failed to init local timer\n");
		goto error;
	}
#ifdef TCP_FD_CACHE
	if (tcp_options.fd_cache) tcp_fd_cache_init();
#endif /* TCP_FD_CACHE */
	
	/* add all the sockets we listen on for connections */
	for (si=tcp_listen; si; si=si->next){
		if ((si->proto==PROTO_TCP) &&(si->socket!=-1)){
			if (io_watch_add(&io_h, si->socket, POLLIN, F_SOCKINFO, si)<0){
				LOG(L_CRIT, "ERROR: tcp_main_loop: init: failed to add "
							"listen socket to the fd list\n");
				goto error;
			}
		}else{
			LOG(L_CRIT, "BUG: tcp_main_loop: non tcp address in tcp_listen\n");
		}
	}
#ifdef USE_TLS
	if (!tls_disable && tls_loaded()){
		for (si=tls_listen; si; si=si->next){
			if ((si->proto==PROTO_TLS) && (si->socket!=-1)){
				if (io_watch_add(&io_h, si->socket, POLLIN, F_SOCKINFO, si)<0){
					LOG(L_CRIT, "ERROR: tcp_main_loop: init: failed to add "
							"tls listen socket to the fd list\n");
					goto error;
				}
			}else{
				LOG(L_CRIT, "BUG: tcp_main_loop: non tls address"
						" in tls_listen\n");
			}
		}
	}
#endif
	/* add all the unix sockets used for communcation with other ser processes
	 *  (get fd, new connection a.s.o) */
	for (r=1; r<process_no; r++){
		if (pt[r].unix_sock>0) /* we can't have 0, we never close it!*/
			if (io_watch_add(&io_h, pt[r].unix_sock, POLLIN,F_PROC, &pt[r])<0){
					LOG(L_CRIT, "ERROR: tcp_main_loop: init: failed to add "
							"process %d unix socket to the fd list\n", r);
					goto error;
			}
	}
	/* add all the unix sokets used for communication with the tcp childs */
	for (r=0; r<tcp_children_no; r++){
		if (tcp_children[r].unix_sock>0)/*we can't have 0, we never close it!*/
			if (io_watch_add(&io_h, tcp_children[r].unix_sock, POLLIN,
									F_TCPCHILD, &tcp_children[r]) <0){
				LOG(L_CRIT, "ERROR: tcp_main_loop: init: failed to add "
						"tcp child %d unix socket to the fd list\n", r);
				goto error;
			}
	}


	/* initialize the cfg framework */
	if (cfg_child_init()) goto error;

	/* main loop */
	switch(io_h.poll_method){
		case POLL_POLL:
			while(1){
				/* wait and process IO */
				io_wait_loop_poll(&io_h, TCP_MAIN_SELECT_TIMEOUT, 0); 
				send_fd_queue_run(&send2child_q); /* then new io */
				/* remove old connections */
				tcp_timer_run();
			}
			break;
#ifdef HAVE_SELECT
		case POLL_SELECT:
			while(1){
				io_wait_loop_select(&io_h, TCP_MAIN_SELECT_TIMEOUT, 0);
				send_fd_queue_run(&send2child_q); /* then new io */
				tcp_timer_run();
			}
			break;
#endif
#ifdef HAVE_SIGIO_RT
		case POLL_SIGIO_RT:
			while(1){
				io_wait_loop_sigio_rt(&io_h, TCP_MAIN_SELECT_TIMEOUT);
				send_fd_queue_run(&send2child_q); /* then new io */
				tcp_timer_run();
			}
			break;
#endif
#ifdef HAVE_EPOLL
		case POLL_EPOLL_LT:
			while(1){
				io_wait_loop_epoll(&io_h, TCP_MAIN_SELECT_TIMEOUT, 0);
				send_fd_queue_run(&send2child_q); /* then new io */
				tcp_timer_run();
			}
			break;
		case POLL_EPOLL_ET:
			while(1){
				io_wait_loop_epoll(&io_h, TCP_MAIN_SELECT_TIMEOUT, 1);
				send_fd_queue_run(&send2child_q); /* then new io */
				tcp_timer_run();
			}
			break;
#endif
#ifdef HAVE_KQUEUE
		case POLL_KQUEUE:
			while(1){
				io_wait_loop_kqueue(&io_h, TCP_MAIN_SELECT_TIMEOUT, 0);
				send_fd_queue_run(&send2child_q); /* then new io */
				tcp_timer_run();
			}
			break;
#endif
#ifdef HAVE_DEVPOLL
		case POLL_DEVPOLL:
			while(1){
				io_wait_loop_devpoll(&io_h, TCP_MAIN_SELECT_TIMEOUT, 0);
				send_fd_queue_run(&send2child_q); /* then new io */
				tcp_timer_run();
			}
			break;
#endif
		default:
			LOG(L_CRIT, "BUG: tcp_main_loop: no support for poll method "
					" %s (%d)\n", 
					poll_method_name(io_h.poll_method), io_h.poll_method);
			goto error;
	}
error:
#ifdef SEND_FD_QUEUE
	destroy_send_fd_queues();
#endif
	destroy_io_wait(&io_h);
	LOG(L_CRIT, "ERROR: tcp_main_loop: exiting...");
	exit(-1);
}



/* cleanup before exit */
void destroy_tcp()
{
		if (tcpconn_id_hash){
			if (tcpconn_lock)
				TCPCONN_UNLOCK; /* hack: force-unlock the tcp lock in case
								   some process was terminated while holding 
								   it; this will allow an almost gracious 
								   shutdown */
			tcpconn_destroy_all(); 
			shm_free(tcpconn_id_hash);
			tcpconn_id_hash=0;
		}
		if (tcp_connections_no){
			shm_free(tcp_connections_no);
			tcp_connections_no=0;
		}
#ifdef TCP_BUF_WRITE
		if (tcp_total_wq){
			shm_free(tcp_total_wq);
			tcp_total_wq=0;
		}
#endif /* TCP_BUF_WRITE */
		if (connection_id){
			shm_free(connection_id);
			connection_id=0;
		}
		if (tcpconn_aliases_hash){
			shm_free(tcpconn_aliases_hash);
			tcpconn_aliases_hash=0;
		}
		if (tcpconn_lock){
			lock_destroy(tcpconn_lock);
			lock_dealloc((void*)tcpconn_lock);
			tcpconn_lock=0;
		}
		if (tcp_children){
			pkg_free(tcp_children);
			tcp_children=0;
		}
		destroy_local_timer(&tcp_main_ltimer);
}



int init_tcp()
{
	char* poll_err;
	
	tcp_options_check();
	/* init lock */
	tcpconn_lock=lock_alloc();
	if (tcpconn_lock==0){
		LOG(L_CRIT, "ERROR: init_tcp: could not alloc lock\n");
		goto error;
	}
	if (lock_init(tcpconn_lock)==0){
		LOG(L_CRIT, "ERROR: init_tcp: could not init lock\n");
		lock_dealloc((void*)tcpconn_lock);
		tcpconn_lock=0;
		goto error;
	}
	/* init globals */
	tcp_connections_no=shm_malloc(sizeof(int));
	if (tcp_connections_no==0){
		LOG(L_CRIT, "ERROR: init_tcp: could not alloc globals\n");
		goto error;
	}
	*tcp_connections_no=0;
	connection_id=shm_malloc(sizeof(int));
	if (connection_id==0){
		LOG(L_CRIT, "ERROR: init_tcp: could not alloc globals\n");
		goto error;
	}
	*connection_id=1;
#ifdef TCP_BUF_WRITE
	tcp_total_wq=shm_malloc(sizeof(*tcp_total_wq));
	if (tcp_total_wq==0){
		LOG(L_CRIT, "ERROR: init_tcp: could not alloc globals\n");
		goto error;
	}
#endif /* TCP_BUF_WRITE */
	/* alloc hashtables*/
	tcpconn_aliases_hash=(struct tcp_conn_alias**)
			shm_malloc(TCP_ALIAS_HASH_SIZE* sizeof(struct tcp_conn_alias*));
	if (tcpconn_aliases_hash==0){
		LOG(L_CRIT, "ERROR: init_tcp: could not alloc address hashtable\n");
		goto error;
	}
	tcpconn_id_hash=(struct tcp_connection**)shm_malloc(TCP_ID_HASH_SIZE*
								sizeof(struct tcp_connection*));
	if (tcpconn_id_hash==0){
		LOG(L_CRIT, "ERROR: init_tcp: could not alloc id hashtable\n");
		goto error;
	}
	/* init hashtables*/
	memset((void*)tcpconn_aliases_hash, 0, 
			TCP_ALIAS_HASH_SIZE * sizeof(struct tcp_conn_alias*));
	memset((void*)tcpconn_id_hash, 0, 
			TCP_ID_HASH_SIZE * sizeof(struct tcp_connection*));
	
	/* fix config variables */
	if (tcp_connect_timeout<0)
		tcp_connect_timeout=DEFAULT_TCP_CONNECT_TIMEOUT;
	if (tcp_send_timeout<0)
		tcp_send_timeout=DEFAULT_TCP_SEND_TIMEOUT;
	if (tcp_con_lifetime<0){
		/* set to max value (~ 1/2 MAX_INT) */
		tcp_con_lifetime=MAX_TCP_CON_LIFETIME;
	}else{
		if ((unsigned)tcp_con_lifetime > 
				(unsigned)TICKS_TO_S(MAX_TCP_CON_LIFETIME)){
			LOG(L_WARN, "init_tcp: tcp_con_lifetime too big (%u s), "
					" the maximum value is %u\n", tcp_con_lifetime,
					TICKS_TO_S(MAX_TCP_CON_LIFETIME));
			tcp_con_lifetime=MAX_TCP_CON_LIFETIME;
		}else{
			tcp_con_lifetime=S_TO_TICKS(tcp_con_lifetime);
		}
	}
	
		poll_err=check_poll_method(tcp_poll_method);
	
	/* set an appropriate poll method */
	if (poll_err || (tcp_poll_method==0)){
		tcp_poll_method=choose_poll_method();
		if (poll_err){
			LOG(L_ERR, "ERROR: init_tcp: %s, using %s instead\n",
					poll_err, poll_method_name(tcp_poll_method));
		}else{
			LOG(L_INFO, "init_tcp: using %s as the io watch method"
					" (auto detected)\n", poll_method_name(tcp_poll_method));
		}
	}else{
			LOG(L_INFO, "init_tcp: using %s io watch method (config)\n",
					poll_method_name(tcp_poll_method));
	}
	
	return 0;
error:
	/* clean-up */
	destroy_tcp();
	return -1;
}


#ifdef TCP_CHILD_NON_BLOCKING
/* returns -1 on error */
static int set_non_blocking(int s)
{
	int flags;
	/* non-blocking */
	flags=fcntl(s, F_GETFL);
	if (flags==-1){
		LOG(L_ERR, "ERROR: set_non_blocking: fnctl failed: (%d) %s\n",
				errno, strerror(errno));
		goto error;
	}
	if (fcntl(s, F_SETFL, flags|O_NONBLOCK)==-1){
		LOG(L_ERR, "ERROR: set_non_blocking: fcntl: set non-blocking failed:"
				" (%d) %s\n", errno, strerror(errno));
		goto error;
	}
	return 0;
error:
	return -1;
}

#endif


/*  returns -1 on error, 0 on success */
int tcp_fix_child_sockets(int* fd)
{
#ifdef TCP_CHILD_NON_BLOCKING
	if ((set_non_blocking(fd[0])<0) ||
		(set_non_blocking(fd[1])<0)){
		return -1;
	}
#endif
	return 0;
}



/* starts the tcp processes */
int tcp_init_children()
{
	int r;
	int reader_fd_1; /* for comm. with the tcp children read  */
	pid_t pid;
	struct socket_info *si;
	
	/* estimate max fd. no:
	 * 1 tcp send unix socket/all_proc, 
	 *  + 1 udp sock/udp proc + 1 tcp_child sock/tcp child*
	 *  + no_listen_tcp */
	for(r=0, si=tcp_listen; si; si=si->next, r++);
#ifdef USE_TLS
	if (! tls_disable)
		for (si=tls_listen; si; si=si->next, r++);
#endif
	
	register_fds(r+tcp_max_connections+get_max_procs()-1 /* tcp main */);
#if 0
	tcp_max_fd_no=get_max_procs()*2 +r-1 /* timer */ +3; /* stdin/out/err*/
	/* max connections can be temporarily exceeded with estimated_process_count
	 * - tcp_main (tcpconn_connect called simultaneously in all all the 
	 *  processes) */
	tcp_max_fd_no+=tcp_max_connections+get_max_procs()-1 /* tcp main */;
#endif
	/* alloc the children array */
	tcp_children=pkg_malloc(sizeof(struct tcp_child)*tcp_children_no);
	if (tcp_children==0){
			LOG(L_ERR, "ERROR: tcp_init_children: out of memory\n");
			goto error;
	}
	/* create the tcp sock_info structures */
	/* copy the sockets --moved to main_loop*/
	
	/* fork children & create the socket pairs*/
	for(r=0; r<tcp_children_no; r++){
		child_rank++;
		pid=fork_tcp_process(child_rank, "tcp receiver", r, &reader_fd_1);
		if (pid<0){
			LOG(L_ERR, "ERROR: tcp_main: fork failed: %s\n",
					strerror(errno));
			goto error;
		}else if (pid>0){
			/* parent */
		}else{
			/* child */
			bind_address=0; /* force a SEGFAULT if someone uses a non-init.
							   bind address on tcp */
			tcp_receive_loop(reader_fd_1);
		}
	}
	return 0;
error:
	return -1;
}



void tcp_get_info(struct tcp_gen_info *ti)
{
	ti->tcp_readers=tcp_children_no;
	ti->tcp_max_connections=tcp_max_connections;
	ti->tcp_connections_no=*tcp_connections_no;
#ifdef TCP_BUF_WRITE
	ti->tcp_write_queued=*tcp_total_wq;
#else
	ti->tcp_write_queued=0;
#endif /* TCP_BUF_WRITE */
}

#endif
