/*
 * $Id: forward.c,v 1.27 2001/11/28 23:25:35 bogdan Exp $
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "forward.h"
#include "config.h"
#include "msg_parser.h"
#include "route.h"
#include "dprint.h"
#include "udp_server.h"
#include "globals.h"
#include "data_lump.h"
#include "ut.h"
#include "mem.h"
#include "msg_translator.h"

#ifdef DEBUG_DMALLOC
#include <dmalloc.h>
#endif

#define MAX_VIA_LINE_SIZE      240
#define MAX_RECEIVED_SIZE  57

#ifdef STATS
#include "stats.h"
#endif



int forward_request( struct sip_msg* msg, struct proxy_l * p)
{
	unsigned int len;
	char* buf;
	struct sockaddr_in* to;


	buf = build_buf_from_sip_request( msg, &len);
	if (!buf){
		LOG(L_ERR, "ERROR: forward_reply: building failed\n");
		goto error;
	}

	to=0;
	to=(struct sockaddr_in*)malloc(sizeof(struct sockaddr));
	if (to==0){
		LOG(L_ERR, "ERROR: forward_reply: out of memory\n");
		goto error;
	}

	 /* send it! */
	DBG("Sending:\n%s.\n", buf);
	DBG("orig. len=%d, new_len=%d\n", msg->len, len );

	to->sin_family = AF_INET;
	to->sin_port = (p->port)?htons(p->port):htons(SIP_PORT);
	/* if error try next ip address if possible */
	if (p->ok==0){
		if (p->host.h_addr_list[p->addr_idx+1])
			p->addr_idx++;
		p->ok=1;
	}
	/* ? not 64bit clean?*/
	to->sin_addr.s_addr=*((long*)p->host.h_addr_list[p->addr_idx]);

	p->tx++;
	p->tx_bytes+=len;

	if (udp_send( buf, len, (struct sockaddr*) to,
				sizeof(struct sockaddr_in))==-1){
			p->errors++;
			p->ok=0;
#ifdef STATS
			update_fail_on_send;
#endif
			goto error;
	}
#ifdef STATS
	/* sent requests stats */
	else update_sent_request( msg->first_line.u.request.method_value );
#endif
	free(buf);
	free(to);
	/* received_buf & line_buf will be freed in receiv_msg by free_lump_list*/
	return 0;
error:
	if (buf) free(buf);
	if (to) free(to);
	return -1;
}



/* removes first via & sends msg to the second */
int forward_reply(struct sip_msg* msg)
{


	unsigned int new_len, via_len,r;
	char* new_buf;
	unsigned offset, s_offset, size;
	struct hostent* he;
	struct sockaddr_in* to;
	char* orig;
	char* buf;
	unsigned int len;
#ifdef DNS_IP_HACK
	int err;
#endif


	orig=msg->orig;
	buf=msg->buf;
	len=msg->len;
	new_buf=0;
	to=0;
	to=(struct sockaddr_in*)malloc(sizeof(struct sockaddr));
	if (to==0){
		LOG(L_ERR, "ERROR: forward_reply: out of memory\n");
		goto error;
	}

	/*check if first via host = us */
	if (check_via){
		for (r=0; r<addresses_no; r++)
			if(strcmp(msg->via1->host.s, names[r])==0) break;
		if (r==addresses_no){
			LOG(L_NOTICE, "ERROR: forward_reply: host in first via!=me :"
					" %s\n", msg->via1->host);
			/* send error msg back? */
			goto error;
		}
	}
	/* we must remove the first via */
	via_len=msg->via1->bsize;
	size=msg->via1->hdr.s-buf;
	DBG("via len: %d, initial size: %d\n", via_len, size);
	if (msg->via1->next){
		/* add hdr size*/
		size+=msg->via1->hdr.len+1;
	    DBG(" adjusted via len: %d, initial size: %d\n",
				via_len, size);
	}else{
		/* add hdr size ("Via:")*/
		via_len+=msg->via1->hdr.len+1;
	}
	new_len=len-via_len;

	DBG(" old size: %d, new size: %d\n", len, new_len);
	new_buf=(char*)malloc(new_len+1);/* +1 is for debugging
											(\0 to print it )*/
	if (new_buf==0){
		LOG(L_ERR, "ERROR: forward_reply: out of memory\n");
		goto error;
	}
	new_buf[new_len]=0; /* debug: print the message */
	memcpy(new_buf, orig, size);
	offset=size;
	s_offset=size+via_len;
	memcpy(new_buf+offset,orig+s_offset, len-s_offset);
	 /* send it! */
	DBG(" copied size: orig:%d, new: %d, rest: %d\n",
			s_offset, offset,
			len-s_offset );
	DBG("Sending: to %s:%d, \n%s.\n",
			msg->via2->host.s,
			(unsigned short)msg->via2->port,
			new_buf);

#ifdef DNS_IP_HACK
	to->sin_addr.s_addr=str2ip(msg->via2->host.s, msg->via2->host.len, &err);
	if (err==0){
		to->sin_family = AF_INET;
		to->sin_port = (msg->via2->port)?htons(msg->via2->port):
						htons(SIP_PORT);
	}else{
#endif
		/* fork? gethostbyname will probably block... */
		he=gethostbyname(msg->via2->host.s);
		if (he==0){
			LOG(L_NOTICE, "ERROR:forward_reply:gethostbyname(%s) failure\n",
					msg->via2->host.s);
			goto error;
		}
		to->sin_family = AF_INET;
		to->sin_port = (msg->via2->port)?htons(msg->via2->port):
						htons(SIP_PORT);
		to->sin_addr.s_addr=*((long*)he->h_addr_list[0]);

#ifdef DNS_IP_HACK
	}
#endif
	if (udp_send(new_buf,new_len, (struct sockaddr*) to,
					sizeof(struct sockaddr_in))==-1)
	{
#ifdef STATS
		update_fail_on_send;
#endif
		goto error;
	}
#ifdef STATS
	else update_sent_response(  msg->first_line.u.reply.statusclass );
#endif
	
	free(new_buf);
	free(to);
	return 0;
error:
	if (new_buf) free(new_buf);
	if (to) free(to);
	return -1;
}
