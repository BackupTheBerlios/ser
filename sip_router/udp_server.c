/*
 * $Id: udp_server.c,v 1.10 2001/11/12 19:51:59 jku Exp $
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>


#include "udp_server.h"
#include "config.h"
#include "dprint.h"
#include "receive.h"

#ifdef DEBUG_DMALLOC
#include <dmalloc.h>
#endif

int udp_sock;



int udp_init(unsigned long ip, unsigned short port)
{
	struct sockaddr_in* addr;
	int optval, optvallen;
	int ioptval, ioptvallen;
	int foptval, foptvallen;
	int voptval, voptvallen;
	int i;
	int phase=0;


	addr=(struct sockaddr_in*)malloc(sizeof(struct sockaddr));
	if (addr==0){
		LOG(L_ERR, "ERROR: udp_init: out of memory\n");
		goto error;
	}
	addr->sin_family=AF_INET;
	addr->sin_port=htons(port);
	addr->sin_addr.s_addr=ip;

	udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (udp_sock==-1){
		LOG(L_ERR, "ERROR: udp_init: socket: %s\n", strerror(errno));
		goto error;
	}
	/* set sock opts? */
	optval=1;
	if (setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR,
					(void*)&optval, sizeof(optval)) ==-1)
	{
		LOG(L_ERR, "ERROR: udp_init: setsockopt: %s\n", strerror(errno));
		goto error;
	}

	/* jku: try to increase buffer size as much as we can */
	ioptvallen=sizeof(ioptval);
	if (getsockopt( udp_sock, SOL_SOCKET, SO_RCVBUF, (void*) &ioptval,
		    &ioptvallen) == -1 )
	{
		LOG(L_ERR, "ERROR: udp_init: getsockopt: %s\n", strerror(errno));
		goto error;
	}
	if ( ioptval==0 ) 
	{
		LOG(L_DBG, "DEBUG: udp_init: SO_RCVBUF initialy set to 0; resetting to %d\n",
			BUFFER_INCREMENT );
		ioptval=BUFFER_INCREMENT;
	} else LOG(L_INFO, "INFO: udp_init: SO_RCVBUF is initially %d\n", ioptval );
	for (optval=ioptval; optval < MAX_RECV_BUFFER_SIZE ;  ) {
		/* increase size; double in initial phase, add linearly later */
		if (phase==0) optval <<= 1; else optval+=BUFFER_INCREMENT;
		LOG(L_DBG, "DEBUG: udp_init: trying SO_RCVBUF: %d\n", optval );
        	if (setsockopt( udp_sock, SOL_SOCKET, SO_RCVBUF,
                             (void*)&optval, sizeof(optval)) ==-1)
        	{
			LOG(L_DBG, "DEBUG: udp_init: SOL_SOCKET failed for %d, phase %d: %s\n",
			    optval,  phase, strerror(errno) );
			/* if setting buffer size failed and still in the aggressive
			   phase, try less agressively; otherwise give up 
			*/
			if (phase==0) { phase=1; optval >>=1 ; continue; } 
			else break;
        	} 
		/* verify if change has taken effect */
		voptvallen=sizeof(voptval);
		if (getsockopt( udp_sock, SOL_SOCKET, SO_RCVBUF, (void*) &voptval,
		    &voptvallen) == -1 )
		{
			LOG(L_ERR, "ERROR: udp_init: getsockopt: %s\n", strerror(errno));
			goto error;
		} else {
			LOG(L_DBG, "DEBUG: setting SO_RCVBUF; set=%d,verify=%d\n", 
				optval, voptval);
			if (voptval<optval) {
				LOG(L_DBG, "DEBUG: setting SO_RCVBUF has no effect\n");
				/* if setting buffer size failed and still in the aggressive
			   	phase, try less agressively; otherwise give up 
				*/
                        	if (phase==0) { phase=1; optval >>=1 ; continue; } 
                        	else break;
			}
		}

	} /* for ... */
	foptvallen=sizeof(foptval);
	if (getsockopt( udp_sock, SOL_SOCKET, SO_RCVBUF, (void*) &foptval,
		    &foptvallen) == -1 )
	{
		LOG(L_ERR, "ERROR: udp_init: getsockopt: %s\n", strerror(errno));
		goto error;
	}
 	LOG(L_INFO, "INFO: udp_init: SO_RCVBUF is finally %d\n", foptval );


	/* EoJKU */

	if (bind(udp_sock, (struct sockaddr*) addr, sizeof(struct sockaddr))==-1){
		LOG(L_ERR, "ERROR: udp_init: bind: %s\n", strerror(errno));
		goto error;
	}

	free(addr);
	return 0;

error:
	if (addr) free(addr);
	return -1;
}



int udp_rcv_loop()
{
	unsigned len;
	char buf[BUF_SIZE+1];
	struct sockaddr* from;
	int fromlen;

	from=(struct sockaddr*) malloc(sizeof(struct sockaddr));
	if (from==0){
		LOG(L_ERR, "ERROR: udp_rcv_loop: out of memory\n");
		goto error;
	}

	for(;;){
		fromlen=sizeof(struct sockaddr);
		len=recvfrom(udp_sock, buf, BUF_SIZE, 0, from, &fromlen);
		if (len==-1){
			LOG(L_ERR, "ERROR: udp_rcv_loop:recvfrom: %s\n",
						strerror(errno));
			if (errno==EINTR)	goto skip;
			else goto error;
		}
		/*debugging, make print* msg work */
		buf[len+1]=0;

		receive_msg(buf, len, ((struct sockaddr_in*)from)->sin_addr.s_addr);
		
	skip: /* do other stuff */
		
	}
	
	if (from) free(from);
	return 0;
	
error:
	if (from) free(from);
	return -1;
}



/* which socket to use? main socket or new one? */
int udp_send(char *buf, unsigned len, struct sockaddr*  to, unsigned tolen)
{

	int n;
again:
	n=sendto(udp_sock, buf, len, 0, to, tolen);
	if (n==-1){
		LOG(L_ERR, "ERROR: udp_send: sendto: %s\n", strerror(errno));
		if (errno==EINTR) goto again;
	}
	return n;
}
