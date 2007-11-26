/* $Id: resolve.c,v 1.27 2007/11/26 17:28:54 mma Exp $*/
/*
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
 * -------
 *  2003-02-13  added proto to sip_resolvehost, for SRV lookups (andrei)
 *  2003-07-03  default port value set according to proto (andrei)
 *  2005-07-11  added resolv_init (timeouts a.s.o) (andrei)
 *  2006-04-13  added sip_hostport2su()  (andrei)
 *  2006-07-13  rdata structures put on diet (andrei)
 *  2006-07-17  rdata contains now also the record name (andrei)
 *  2006-08-18  get_record can append also the additional records to the
 *               returned list (andrei)
 *  2007-06-15  naptr support (andrei)
 *  2007-10-10  short name resolution using search list supported (mma)
 *              set dns_use_search_list=1 (default on)
 *              new option dns_search_full_match (default on) controls
 *              whether rest of the name is matched against search list
 *              or blindly accepted (better performance but exploitable)
 */ 


#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <string.h>

#include "resolve.h"
#include "compiler_opt.h"
#include "dprint.h"
#include "mem/mem.h"
#include "ip_addr.h"
#include "error.h"
#include "globals.h" /* tcp_disable, tls_disable a.s.o */

#ifdef USE_DNS_CACHE
#include "dns_cache.h"
#endif



/* mallocs for local stuff */
#define local_malloc pkg_malloc
#define local_free   pkg_free

#ifdef USE_IPV6
int dns_try_ipv6=1; /* default on */
#else
int dns_try_ipv6=0; /* off, if no ipv6 support */
#endif
int dns_try_naptr=0;  /* off by default */

int dns_udp_pref=3;  /* udp transport preference (for naptr) */
int dns_tcp_pref=2;  /* tcp transport preference (for naptr) */
int dns_tls_pref=1;  /* tls transport preference (for naptr) */

#ifdef USE_NAPTR
#define PROTO_LAST  PROTO_SCTP
static int naptr_proto_pref[PROTO_LAST];
#endif

/* declared in globals.h */
int dns_retr_time=-1;
int dns_retr_no=-1;
int dns_servers_no=-1;
int dns_search_list=-1;
int dns_search_fmatch=-1;

#ifdef USE_NAPTR
void init_naptr_proto_prefs()
{
	if ((PROTO_UDP >= PROTO_LAST) || (PROTO_TCP >= PROTO_LAST) ||
		(PROTO_TLS >= PROTO_LAST)){
		BUG("init_naptr_proto_prefs: array too small \n");
		return;
	}
	naptr_proto_pref[PROTO_UDP]=dns_udp_pref;
	naptr_proto_pref[PROTO_TCP]=dns_tcp_pref;
	naptr_proto_pref[PROTO_TLS]=dns_tls_pref;
}

#endif /* USE_NAPTR */



/* init. the resolver
 * params: retr_time  - time before retransmitting (must be >0)
 *         retr_no    - retransmissions number
 *         servers_no - how many dns servers will be used
 *                      (from the one listed in /etc/resolv.conf)
 *         search     - if 0 the search list in /etc/resolv.conf will
 *                      be ignored (HINT: even if you don't have a
 *                      search list in resolv.conf, it's still better
 *                      to set search to 0, because an empty seachlist
 *                      means in fact search "" => it takes more time)
 * If any of the parameters <0, the default (system specific) value
 * will be used. See also resolv.conf(5).
 * returns: 0 on success, -1 on error
 */
int resolv_init()
{
	res_init();
#ifdef HAVE_RESOLV_RES
	if (dns_retr_time>0)
		_res.retrans=dns_retr_time;
	if (dns_retr_no>0)
		_res.retry=dns_retr_no;
	if (dns_servers_no>=0)
		_res.nscount=dns_servers_no;
	if (dns_search_list==0)
		_res.options&=~(RES_DEFNAMES|RES_DNSRCH);
#else
#warning "no resolv timeout support"
	LOG(L_WARN, "WARNING: resolv_init: no resolv options support - resolv"
			" options will be ignored\n");
#endif
#ifdef USE_NAPTR
	init_naptr_proto_prefs();
#endif
	return 0;
}



 /*  skips over a domain name in a dns message
 *  (it can be  a sequence of labels ending in \0, a pointer or
 *   a sequence of labels ending in a pointer -- see rfc1035
 *   returns pointer after the domain name or null on error*/
unsigned char* dns_skipname(unsigned char* p, unsigned char* end)
{
	while(p<end){
		/* check if \0 (root label length) */
		if (*p==0){
			p+=1;
			break;
		}
		/* check if we found a pointer */
		if (((*p)&0xc0)==0xc0){
			/* if pointer skip over it (2 bytes) & we found the end */
			p+=2;
			break;
		}
		/* normal label */
		p+=*p+1;
	}
	return (p>end)?0:p;
}



/* parses the srv record into a srv_rdata structure
 *   msg   - pointer to the dns message
 *   end   - pointer to the end of the message
 *   rdata - pointer  to the rdata part of the srv answer
 * returns 0 on error, or a dyn. alloc'ed srv_rdata structure */
/* SRV rdata format:
 *            111111
 *  0123456789012345
 * +----------------+
 * |     priority   |
 * |----------------|
 * |     weight     |
 * |----------------|
 * |   port number  |
 * |----------------|
 * |                |
 * ~      name      ~
 * |                |
 * +----------------+
 */
struct srv_rdata* dns_srv_parser( unsigned char* msg, unsigned char* end,
								  unsigned char* rdata)
{
	struct srv_rdata* srv;
	unsigned short priority;
	unsigned short weight;
	unsigned short port;
	int len;
	char name[MAX_DNS_NAME];
	
	srv=0;
	if ((rdata+6+1)>end) goto error;
	
	memcpy((void*)&priority, rdata, 2);
	memcpy((void*)&weight,   rdata+2, 2);
	memcpy((void*)&port,     rdata+4, 2);
	rdata+=6;
	if (dn_expand(msg, end, rdata, name, MAX_DNS_NAME-1)<0)
		goto error;
	len=strlen(name);
	if (len>255)
		goto error;
	/* alloc enought space for the struct + null terminated name */
	srv=local_malloc(sizeof(struct srv_rdata)-1+len+1);
	if (srv==0){
		LOG(L_ERR, "ERROR: dns_srv_parser: out of memory\n");
		goto error;
	}
	srv->priority=ntohs(priority);
	srv->weight=ntohs(weight);
	srv->port=ntohs(port);
	srv->name_len=len;
	memcpy(srv->name, name, srv->name_len);
	srv->name[srv->name_len]=0;
	
	return srv;
error:
	if (srv) local_free(srv);
	return 0;
}


/* parses the naptr record into a naptr_rdata structure
 *   msg   - pointer to the dns message
 *   end   - pointer to the end of the message
 *   rdata - pointer  to the rdata part of the naptr answer
 * returns 0 on error, or a dyn. alloc'ed naptr_rdata structure */
/* NAPTR rdata format:
 *            111111
 *  0123456789012345
 * +----------------+
 * |      order     |
 * |----------------|
 * |   preference   |
 * |----------------|
 * ~     flags      ~
 * |   (string)     |
 * |----------------|
 * ~    services    ~
 * |   (string)     |
 * |----------------|
 * ~    regexp      ~
 * |   (string)     |
 * |----------------|
 * ~  replacement   ~
   |    (name)      |
 * +----------------+
 */
struct naptr_rdata* dns_naptr_parser( unsigned char* msg, unsigned char* end,
								  unsigned char* rdata)
{
	struct naptr_rdata* naptr;
	unsigned char* flags;
	unsigned char* services;
	unsigned char* regexp;
	unsigned short order;
	unsigned short pref;
	unsigned char flags_len;
	unsigned char services_len;
	unsigned char regexp_len;
	int len;
	char repl[MAX_DNS_NAME];
	
	naptr = 0;
	if ((rdata + 7 + 1)>end) goto error;
	
	memcpy((void*)&order, rdata, 2);
	memcpy((void*)&pref, rdata + 2, 2);
	flags_len = rdata[4];
	if ((rdata + 7 + 1 +  flags_len) > end)
		goto error;
	flags=rdata+5;
	services_len = rdata[5 + flags_len];
	if ((rdata + 7 + 1 + flags_len + services_len) > end)
		goto error;
	services=rdata + 6 + flags_len;
	regexp_len = rdata[6 + flags_len + services_len];
	if ((rdata + 7 +1 + flags_len + services_len + regexp_len) > end)
		goto error;
	regexp=rdata + 7 + flags_len + services_len;
	rdata = rdata + 7 + flags_len + services_len + regexp_len;
	if (dn_expand(msg, end, rdata, repl, MAX_DNS_NAME-1) == -1)
		goto error;
	len=strlen(repl);
	if (len>255)
		goto error;
	naptr=local_malloc(sizeof(struct naptr_rdata)+flags_len+services_len+
						regexp_len+len+1-1);
	if (naptr == 0){
		LOG(L_ERR, "ERROR: dns_naptr_parser: out of memory\n");
		goto error;
	}
	naptr->order=ntohs(order);
	naptr->pref=ntohs(pref);
	
	naptr->flags=&naptr->str_table[0];
	naptr->flags_len=flags_len;
	memcpy(naptr->flags, flags, naptr->flags_len);
	naptr->services=&naptr->str_table[flags_len];
	naptr->services_len=services_len;
	memcpy(naptr->services, services, naptr->services_len);
	naptr->regexp=&naptr->str_table[flags_len+services_len];
	naptr->regexp_len=regexp_len;
	memcpy(naptr->regexp, regexp, naptr->regexp_len);
	naptr->repl=&naptr->str_table[flags_len+services_len+regexp_len];
	naptr->repl_len=len;
	memcpy(naptr->repl, repl, len);
	naptr->repl[len]=0; /* null term. */
	
	return naptr;
error:
	if (naptr) local_free(naptr);
	return 0;
}



/* parses a CNAME record into a cname_rdata structure */
struct cname_rdata* dns_cname_parser( unsigned char* msg, unsigned char* end,
									  unsigned char* rdata)
{
	struct cname_rdata* cname;
	int len;
	char name[MAX_DNS_NAME];
	
	cname=0;
	if (dn_expand(msg, end, rdata, name, MAX_DNS_NAME-1)==-1)
		goto error;
	len=strlen(name);
	if (len>255)
		goto error;
	/* alloc sizeof struct + space for the null terminated name */
	cname=local_malloc(sizeof(struct cname_rdata)-1+len+1);
	if(cname==0){
		LOG(L_ERR, "ERROR: dns_cname_parser: out of memory\n");
		goto error;
	}
	cname->name_len=len;
	memcpy(cname->name, name, cname->name_len);
	cname->name[cname->name_len]=0;
	return cname;
error:
	if (cname) local_free(cname);
	return 0;
}



/* parses an A record rdata into an a_rdata structure
 * returns 0 on error or a dyn. alloc'ed a_rdata struct
 */
struct a_rdata* dns_a_parser(unsigned char* rdata, unsigned char* end)
{
	struct a_rdata* a;
	
	if (rdata+4>end) goto error;
	a=(struct a_rdata*)local_malloc(sizeof(struct a_rdata));
	if (a==0){
		LOG(L_ERR, "ERROR: dns_a_parser: out of memory\n");
		goto error;
	}
	memcpy(a->ip, rdata, 4);
	return a;
error:
	return 0;
}



/* parses an AAAA (ipv6) record rdata into an aaaa_rdata structure
 * returns 0 on error or a dyn. alloc'ed aaaa_rdata struct */
struct aaaa_rdata* dns_aaaa_parser(unsigned char* rdata, unsigned char* end)
{
	struct aaaa_rdata* aaaa;
	
	if (rdata+16>end) goto error;
	aaaa=(struct aaaa_rdata*)local_malloc(sizeof(struct aaaa_rdata));
	if (aaaa==0){
		LOG(L_ERR, "ERROR: dns_aaaa_parser: out of memory\n");
		goto error;
	}
	memcpy(aaaa->ip6, rdata, 16);
	return aaaa;
error:
	return 0;
}



/* frees completely a struct rdata list */
void free_rdata_list(struct rdata* head)
{
	struct rdata* l;
	struct rdata* next_l;
	l=head;
	while (l != 0) {
		next_l = l->next;
		/* free the parsed rdata*/
		if (l->rdata) local_free(l->rdata);
		local_free(l);
		l = next_l;
	}
}

#ifdef HAVE_RESOLV_RES
/* checks whether supplied name exists in the resolver search list
 * returns 1 if found
 *         0 if not found
 */
int match_search_list(const res_state res, char* name) {
	int i;
	for (i=0; (i<MAXDNSRCH) && (res->dnsrch[i]); i++) {
		if (strcasecmp(name, res->dnsrch[i])==0) 
			return 1;
	}
	return 0;
}
#endif

/* gets the DNS records for name:type
 * returns a dyn. alloc'ed struct rdata linked list with the parsed responses
 * or 0 on error
 * see rfc1035 for the query/response format */
struct rdata* get_record(char* name, int type, int flags)
{
	int size;
	int skip;
	int qno, answers_no;
	int r;
	static union dns_query buff;
	unsigned char* p;
	unsigned char* end;
	static char rec_name[MAX_DNS_NAME]; /* placeholder for the record name */
	int rec_name_len;
	unsigned short rtype, class, rdlength;
	unsigned int ttl;
	struct rdata* head;
	struct rdata** crt;
	struct rdata** last;
	struct rdata* rd;
	struct srv_rdata* srv_rd;
	struct srv_rdata* crt_srv;
	int search_list_used;
	int name_len;
	struct rdata* fullname_rd;
	
	if (dns_search_list==0) {
		search_list_used=0;
		name_len=0;
	} else {
		search_list_used=1;
		name_len=strlen(name);
	}
	fullname_rd=0;

	size=res_search(name, C_IN, type, buff.buff, sizeof(buff));
	if (size<0) {
		DBG("get_record: lookup(%s, %d) failed\n", name, type);
		goto not_found;
	}
	else if (size > sizeof(buff)) size=sizeof(buff);
	head=rd=0;
	last=crt=&head;
	
	p=buff.buff+DNS_HDR_SIZE;
	end=buff.buff+size;
	if (p>=end) goto error_boundary;
	qno=ntohs((unsigned short)buff.hdr.qdcount);

	for (r=0; r<qno; r++){
		/* skip the name of the question */
		if ((p=dns_skipname(p, end))==0) {
			LOG(L_ERR, "ERROR: get_record: skipname==0\n");
			goto error;
		}
		p+=2+2; /* skip QCODE & QCLASS */
	#if 0
		for (;(p<end && (*p)); p++);
		p+=1+2+2; /* skip the ending  '\0, QCODE and QCLASS */
	#endif
		if (p>end) {
			LOG(L_ERR, "ERROR: get_record: p>=end\n");
			goto error;
		}
	};
	answers_no=ntohs((unsigned short)buff.hdr.ancount);
again:
	for (r=0; (r<answers_no) && (p<end); r++){
#if 0
		/*  ignore it the default domain name */
		if ((p=dns_skipname(p, end))==0) {
			LOG(L_ERR, "ERROR: get_record: skip_name=0 (#2)\n");
			goto error;
		}
#else
		if ((skip=dn_expand(buff.buff, end, p, rec_name, MAX_DNS_NAME-1))==-1){
			LOG(L_ERR, "ERROR: get_record: dn_expand(rec_name) failed\n");
			goto error;
		}
#endif
		p+=skip;
		rec_name_len=strlen(rec_name);
		if (rec_name_len>255){
			LOG(L_ERR, "ERROR: get_record: dn_expand(rec_name): name too"
					" long  (%d)\n", rec_name_len);
			goto error;
		}
		/* check if enough space is left for type, class, ttl & size */
		if ((p+2+2+4+2)>end) goto error_boundary;
		/* get type */
		memcpy((void*) &rtype, (void*)p, 2);
		rtype=ntohs(rtype);
		p+=2;
		/* get  class */
		memcpy((void*) &class, (void*)p, 2);
		class=ntohs(class);
		p+=2;
		/* get ttl*/
		memcpy((void*) &ttl, (void*)p, 4);
		ttl=ntohl(ttl);
		p+=4;
		/* get size */
		memcpy((void*)&rdlength, (void*)p, 2);
		rdlength=ntohs(rdlength);
		p+=2;
		if ((flags & RES_ONLY_TYPE) && (rtype!=type)){
			/* skip */
			p+=rdlength;
			continue;
		}
		/* expand the "type" record  (rdata)*/
		
		rd=(struct rdata*) local_malloc(sizeof(struct rdata)+rec_name_len+1-1);
		if (rd==0){
			LOG(L_ERR, "ERROR: get_record: out of memory\n");
			goto error;
		}
		rd->type=rtype;
		rd->class=class;
		rd->ttl=ttl;
		rd->next=0;
		memcpy(rd->name, rec_name, rec_name_len);
		rd->name[rec_name_len]=0;
		rd->name_len=rec_name_len;
		/* check if full name matches */
		if ((search_list_used==1)&&(fullname_rd==0)&&
				(rec_name_len>=name_len)&&
				(strncasecmp(rec_name, name, name_len)==0)) {
			/* now we have record which's name is the same (up-to the name_len
			 * with the searched one
			 * if the length is the same - we found full match, no fake cname
			 *   needed, just clear the flag
			 * if the length of the name differs - it has matched using search list
			 *   remember the rd, so we can create fake CNAME record when all answers
			 *   are used and no better match found
			 */
			if (rec_name_len==name_len)
				search_list_used=0;
			/* this is safe.... here was rec_name_len > name_len */
			else if (rec_name[name_len]=='.') {
#ifdef HAVE_RESOLV_RES
				if ((dns_search_fmatch==0) ||
						(match_search_list(&_res, rec_name+name_len+1)!=0))
#endif
					fullname_rd=rd;
			}
		}
		switch(rtype){
			case T_SRV:
				srv_rd= dns_srv_parser(buff.buff, end, p);
				rd->rdata=(void*)srv_rd;
				if (srv_rd==0) goto error_parse;
				
				/* insert sorted into the list */
				for (crt=&head; *crt; crt= &((*crt)->next)){
					if ((*crt)->type!=T_SRV)
						continue;
					crt_srv=(struct srv_rdata*)(*crt)->rdata;
					if ((srv_rd->priority <  crt_srv->priority) ||
					   ( (srv_rd->priority == crt_srv->priority) && 
							 (srv_rd->weight > crt_srv->weight) ) ){
						/* insert here */
						goto skip;
					}
				}
				last=&(rd->next); /*end of for => this will be the last elem*/
			skip:
				/* insert here */
				rd->next=*crt;
				*crt=rd;
				
				break;
			case T_A:
				rd->rdata=(void*) dns_a_parser(p,end);
				if (rd->rdata==0) goto error_parse;
				*last=rd; /* last points to the last "next" or the list head*/
				last=&(rd->next);
				break;
			case T_AAAA:
				rd->rdata=(void*) dns_aaaa_parser(p,end);
				if (rd->rdata==0) goto error_parse;
				*last=rd;
				last=&(rd->next);
				break;
			case T_CNAME:
				rd->rdata=(void*) dns_cname_parser(buff.buff, end, p);
				if(rd->rdata==0) goto error_parse;
				*last=rd;
				last=&(rd->next);
				break;
			case T_NAPTR:
				rd->rdata=(void*) dns_naptr_parser(buff.buff, end, p);
				if(rd->rdata==0) goto error_parse;
				*last=rd;
				last=&(rd->next);
				break;
			default:
				LOG(L_ERR, "WARNING: get_record: unknown type %d\n", rtype);
				rd->rdata=0;
				*last=rd;
				last=&(rd->next);
		}
		
		p+=rdlength;
		
	}
	if (flags & RES_AR){
		flags&=~RES_AR;
		answers_no=ntohs((unsigned short)buff.hdr.nscount);
#ifdef RESOLVE_DBG
		DBG("get_record: skipping %d NS (p=%p, end=%p)\n", answers_no, p, end);
#endif
		for (r=0; (r<answers_no) && (p<end); r++){
			/* skip over the ns records */
			if ((p=dns_skipname(p, end))==0) {
				LOG(L_ERR, "ERROR: get_record: skip_name=0 (#3)\n");
				goto error;
			}
			/* check if enough space is left for type, class, ttl & size */
			if ((p+2+2+4+2)>end) goto error_boundary;
			memcpy((void*)&rdlength, (void*)p+2+2+4, 2);
			p+=2+2+4+2+ntohs(rdlength);
		}
		answers_no=ntohs((unsigned short)buff.hdr.arcount);
#ifdef RESOLVE_DBG
		DBG("get_record: parsing %d ARs (p=%p, end=%p)\n", answers_no, p, end);
#endif
		goto again; /* add also the additional records */
	}

	/* if the name was expanded using DNS search list
	 * create fake CNAME record to convert the short name
	 * (queried) to long name (answered)
	 */
	if ((search_list_used==1)&&(fullname_rd!=0)) {
		rd=(struct rdata*) local_malloc(sizeof(struct rdata)+name_len+1-1);
		if (rd==0){
			LOG(L_ERR, "ERROR: get_record: out of memory\n");
			goto error;
		}
		rd->type=T_CNAME;
		rd->class=fullname_rd->class;
		rd->ttl=fullname_rd->ttl;
		rd->next=head;
		memcpy(rd->name, name, name_len);
		rd->name[name_len]=0;
		rd->name_len=name_len;
		/* alloc sizeof struct + space for the null terminated name */
		rd->rdata=(void*)local_malloc(sizeof(struct cname_rdata)-1+head->name_len+1);
		if(rd->rdata==0){
			LOG(L_ERR, "ERROR: get_record: out of memory\n");
			goto error_rd;
		}
		((struct cname_rdata*)(rd->rdata))->name_len=fullname_rd->name_len;
		memcpy(((struct cname_rdata*)(rd->rdata))->name, fullname_rd->name, fullname_rd->name_len);
		((struct cname_rdata*)(rd->rdata))->name[head->name_len]=0;
		head=rd;
	}

	return head;
error_boundary:
		LOG(L_ERR, "ERROR: get_record: end of query buff reached\n");
		if (head) free_rdata_list(head);
		return 0;
error_parse:
		LOG(L_ERR, "ERROR: get_record: rdata parse error (%s, %d), %p-%p"
						" rtype=%d, class=%d, ttl=%d, rdlength=%d \n",
				name, type,
				p, end, rtype, class, ttl, rdlength);
error_rd:
		if (rd) local_free(rd); /* rd->rdata=0 & rd is not linked yet into
								   the list */
error:
		LOG(L_ERR, "ERROR: get_record \n");
		if (head) free_rdata_list(head);
not_found:
	return 0;
}

#ifdef USE_NAPTR

/* service matching constants, lowercase */
#define SIP_SCH		0x2b706973
#define SIPS_SCH	0x73706973
#define SIP_D2U		0x00753264
#define SIP_D2T		0x00743264
#define SIP_D2S		0x00733264
#define SIPS_D2T	0x7432642b


/* get protocol from a naptr rdata and check for validity
 * returns > 0 (PROTO_UDP, PROTO_TCP, PROTO_SCTP or PROTO_TLS)
 *         <=0  on error 
 */
char naptr_get_sip_proto(struct naptr_rdata* n)
{
	unsigned int s;
	char proto;

	proto=-1;
	
	if ((n->flags_len!=1) || (*n->flags!='s'))
		return -1;
	if (n->regexp_len!=0)
		return -1;
	/* SIP+D2U, SIP+D2T, SIP+D2S, SIPS+D2T */
	if (n->services_len==7){ /* SIP+D2X */
		s=n->services[0]+(n->services[1]<<8)+(n->services[2]<<16)+
				(n->services[3]<<24);
		s|=0x20202020;
		if (s==SIP_SCH){
			s=n->services[4]+(n->services[5]<<8)+(n->services[6]<<16);
			s|=0x00202020;
			switch(s){
				case SIP_D2U:
					proto=PROTO_UDP;
					break;
				case SIP_D2T:
					proto=PROTO_TCP;
					break;
				case SIP_D2S:
					proto=PROTO_SCTP;
					break;
				default:
					return -1;
			}
		}else{
			return -1;
		}
	}else if  (n->services_len==8){ /*SIPS+D2T */
		s=n->services[0]+(n->services[1]<<8)+(n->services[2]<<16)+
				(n->services[3]<<24);
		s|=0x20202020;
		if (s==SIPS_SCH){
			s=n->services[4]+(n->services[5]<<8)+(n->services[6]<<16)+
					(n->services[7]<<24);
			s|=0x20202020;
			if (s==SIPS_D2T){
				proto=PROTO_TLS;
			}
		}else{
			return -1;
		}
	}else{
		return -1;
	}
	return proto;
}



inline static int proto_pref_score(char proto)
{
	if ((proto>=PROTO_UDP) && (proto<= PROTO_TLS))
		return naptr_proto_pref[(int)proto];
	return 0;
}



/* returns true if we support the protocol */
int naptr_proto_supported(char proto)
{
	if (proto_pref_score(proto)<0)
		return 0;
	switch(proto){
		case PROTO_UDP:
			return 1;
#ifdef USE_TCP
		case PROTO_TCP:
			return !tcp_disable;
#ifdef USE_TLS
		case PROTO_TLS:
			return !tls_disable;
#endif /* USE_TLS */
#endif /* USE_TCP */
		case PROTO_SCTP:
			return 0; /* not supported */
	}
	return 0;
}




/* returns true if new_proto is preferred over old_proto */
int naptr_proto_preferred(char new_proto, char old_proto)
{
	return proto_pref_score(new_proto)>proto_pref_score(old_proto);
}


/* choose between 2 naptr records, should take into account local
 * preferences too
 * returns 1 if the new record was selected, 0 otherwise */
int naptr_choose (struct naptr_rdata** crt, char* crt_proto,
									struct naptr_rdata* n , char n_proto)
{
#ifdef NAPTR_DBG
	DBG("naptr_choose(o: %d w: %d p:%d , o: %d w:%d p:%d)\n",
			*crt?(int)(*crt)->order:-1, *crt?(int)(*crt)->pref:-1,
			(int)*crt_proto,
			(int)n->order, (int)n->pref, (int)n_proto);
#endif
	if ((*crt==0) || ((*crt_proto!=n_proto) && 
						( naptr_proto_preferred(n_proto, *crt_proto))) )
			goto change;
	if ((n->order<(*crt)->order) || ((n->order== (*crt)->order) &&
									(n->pref < (*crt)->pref))){
			goto change;
	}
#ifdef NAPTR_DBG
	DBG("naptr_choose: no change\n");
#endif
	return 0;
change:
#ifdef NAPTR_DBG
	DBG("naptr_choose: changed\n");
#endif
	*crt_proto=n_proto;
	*crt=n;
	return 1;
}
#endif /* USE_NAPTR */



/* internal sip srv resolver: resolves a host name trying:
 * - SRV lookup if the address is not an ip *port==0. The result of the SRV
 *   query will be used for an A/AAAA lookup.
 *  - normal A/AAAA lookup (either fallback from the above or if *port!=0
 *   and *proto!=0 or port==0 && proto==0)
 * when performing SRV lookup (*port==0) it will use *proto to look for
 * tcp or udp hosts, otherwise proto is unused; if proto==0 => no SRV lookup
 * If zt is set, name will be assumed to be 0 terminated and some copy 
 * operations will be avoided.
 * If is_srv is set it will assume name has the srv prefixes for sip already
 *  appended and it's already 0-term'ed; if not it will append them internally.
 * If ars !=0, it will first try to look through them and only if the SRV
 *   record is not found it will try doing a DNS query  (ars will not be
 *   freed, the caller should take care of them)
 * returns: hostent struct & *port filled with the port from the SRV record;
 *  0 on error
 */
struct hostent* srv_sip_resolvehost(str* name, int zt, unsigned short* port,
									char* proto, int is_srv, struct rdata* ars)
{
	struct hostent* he;
	struct ip_addr* ip;
	static char tmp[MAX_DNS_NAME]; /* tmp. buff. for SRV lookups and
	                                  null. term  strings */
	struct rdata* l;
	struct srv_rdata* srv;
	struct rdata* srv_head;
	char* srv_target;
	char srv_proto;

	/* init */
	srv_head=0;
	srv_target=0;
	if (name->len >= MAX_DNS_NAME) {
		LOG(L_ERR, "sip_resolvehost: domain name too long\n");
		he=0;
		goto end;
	}
#ifdef RESOLVE_DBG
	DBG("srv_sip_resolvehost: %.*s:%d proto=%d\n", name->len, name->s,
			port?(int)*port:-1, proto?(int)*proto:-1);
#endif
	if (is_srv){
		/* skip directly to srv resolving */
		srv_proto=(proto)?*proto:0;
		*port=(srv_proto==PROTO_TLS)?SIPS_PORT:SIP_PORT;
		if (zt){
			srv_target=name->s; /* name.s must be 0 terminated in
								  this case */
		}else{
			memcpy(tmp, name->s, name->len);
			tmp[name->len] = '\0';
			srv_target=tmp;
		}
		goto do_srv; /* skip to the actual srv query */
	}
	if (proto){ /* makes sure we have a protocol set*/
		if (*proto==0)
			*proto=srv_proto=PROTO_UDP; /* default */
		else
			srv_proto=*proto;
	}else{
		srv_proto=PROTO_UDP;
	}
	/* try SRV if no port specified (draft-ietf-sip-srv-06) */
	if ((port)&&(*port==0)){
		*port=(srv_proto==PROTO_TLS)?SIPS_PORT:SIP_PORT; /* just in case we
														  don't find another */
		/* check if it's an ip address */
		if (((ip=str2ip(name))!=0)
#ifdef	USE_IPV6
			  || ((ip=str2ip6(name))!=0) 
#endif
			 ){
			/* we are lucky, this is an ip address */
			he=ip_addr2he(name, ip);
			goto end;
		}
		if ((name->len+SRV_MAX_PREFIX_LEN+1)>MAX_DNS_NAME){
			LOG(L_WARN, "WARNING: sip_resolvehost: domain name too long (%d),"
						" unable to perform SRV lookup\n", name->len);
		}else{
			
			switch(srv_proto){
				case PROTO_NONE: /* no proto specified, use udp */
					if (proto)
						*proto=PROTO_UDP;
					/* no break */
				case PROTO_UDP:
					memcpy(tmp, SRV_UDP_PREFIX, SRV_UDP_PREFIX_LEN);
					memcpy(tmp+SRV_UDP_PREFIX_LEN, name->s, name->len);
					tmp[SRV_UDP_PREFIX_LEN + name->len] = '\0';
					break;
				case PROTO_TCP:
					memcpy(tmp, SRV_TCP_PREFIX, SRV_TCP_PREFIX_LEN);
					memcpy(tmp+SRV_TCP_PREFIX_LEN, name->s, name->len);
					tmp[SRV_TCP_PREFIX_LEN + name->len] = '\0';
					break;
				case PROTO_TLS:
					memcpy(tmp, SRV_TLS_PREFIX, SRV_TLS_PREFIX_LEN);
					memcpy(tmp+SRV_TLS_PREFIX_LEN, name->s, name->len);
					tmp[SRV_TLS_PREFIX_LEN + name->len] = '\0';
					break;
				default:
					LOG(L_CRIT, "BUG: sip_resolvehost: unknown proto %d\n",
							srv_proto);
					he=0;
					goto end;
			}
			srv_target=tmp;
do_srv:
			/* try to find the SRV records inside previous ARs  first*/
			for (l=ars; l; l=l->next){
				if (l->type!=T_SRV) continue; 
				srv=(struct srv_rdata*) l->rdata;
				if (srv==0){
					LOG(L_CRIT, "sip_resolvehost: BUG: null rdata\n");
					/* cleanup on exit only */
					break;
				}
				he=resolvehost(srv->name);
				if (he!=0){
					/* we found it*/
#ifdef RESOLVE_DBG
					DBG("sip_resolvehost: found SRV(%s) = %s:%d in AR\n",
							srv_target, srv->name, srv->port);
#endif
					*port=srv->port;
					/* cleanup on exit */
					goto end;
				}
			}
			srv_head=get_record(srv_target, T_SRV, RES_ONLY_TYPE);
			for(l=srv_head; l; l=l->next){
				if (l->type!=T_SRV) continue; /*should never happen*/
				srv=(struct srv_rdata*) l->rdata;
				if (srv==0){
					LOG(L_CRIT, "sip_resolvehost: BUG: null rdata\n");
					/* cleanup on exit only */
					break;
				}
				he=resolvehost(srv->name);
				if (he!=0){
					/* we found it*/
#ifdef RESOLVE_DBG
					DBG("sip_resolvehost: SRV(%s) = %s:%d\n",
							srv_target, srv->name, srv->port);
#endif
					*port=srv->port;
					/* cleanup on exit */
					goto end;
				}
			}
			if (is_srv){
				/* if the name was already into SRV format it doesn't make
				 * any sense to fall back to A/AAAA */
				he=0;
				goto end;
			}
			/* cleanup on exit */
#ifdef RESOLVE_DBG
			DBG("sip_resolvehost: no SRV record found for %.*s," 
					" trying 'normal' lookup...\n", name->len, name->s);
#endif
		}
	}
/*skip_srv:*/
	if (likely(!zt)){
		memcpy(tmp, name->s, name->len);
		tmp[name->len] = '\0';
		he=resolvehost(tmp);
	}else{
		he=resolvehost(name->s);
	}
end:
#ifdef RESOLVE_DBG
	DBG("srv_sip_resolvehost: returning %p (%.*s:%d proto=%d)\n",
			he, name->len, name->s,
			port?(int)*port:-1, proto?(int)*proto:-1);
#endif
	if (srv_head)
		free_rdata_list(srv_head);
	return he;
}



#ifdef USE_NAPTR 


/* iterates over a naptr rr list, returning each time a "good" naptr record
 * is found.( srv type, no regex and a supported protocol)
 * params:
 *         naptr_head - naptr rr list head
 *         tried      - bitmap used to keep track of the already tried records
 *                      (no more then sizeof(tried)*8 valid records are 
 *                      ever walked
 *         srv_name   - if succesfull, it will be set to the selected record
 *                      srv name (naptr repl.)
 *         proto      - if succesfull it will be set to the selected record
 *                      protocol
 * returns  0 if no more records found or a pointer to the selected record
 *  and sets  protocol and srv_name
 * WARNING: when calling first time make sure you run first 
 *           naptr_iterate_init(&tried)
 */
struct rdata* naptr_sip_iterate(struct rdata* naptr_head, 
										naptr_bmp_t* tried,
										str* srv_name, char* proto)
{
	int i, idx;
	struct rdata* l;
	struct rdata* l_saved;
	struct naptr_rdata* naptr;
	struct naptr_rdata* naptr_saved;
	char saved_proto;
	char naptr_proto;

	idx=0;
	naptr_proto=PROTO_NONE;
	naptr_saved=0;
	l_saved=0;
	saved_proto=0;
	i=0;
	for(l=naptr_head; l && (i<MAX_NAPTR_RRS); l=l->next){
		if (l->type!=T_NAPTR) continue; 
		naptr=(struct naptr_rdata*) l->rdata;
		if (naptr==0){
				LOG(L_CRIT, "naptr_iterate: BUG: null rdata\n");
			goto end;
		}
		/* check if valid and get proto */
		if ((naptr_proto=naptr_get_sip_proto(naptr))<=0) continue;
		if (*tried& (1<<i)){
			i++;
			continue; /* already tried */
		}
#ifdef NAPTR_DBG
		DBG("naptr_iterate: found a valid sip NAPTR rr %.*s,"
					" proto %d\n", naptr->repl_len, naptr->repl, 
					(int)naptr_proto);
#endif
		if ((naptr_proto_supported(naptr_proto))){
			if (naptr_choose(&naptr_saved, &saved_proto,
								naptr, naptr_proto))
				idx=i;
				l_saved=l;
			}
		i++;
	}
	if (naptr_saved){
		/* found something */
#ifdef NAPTR_DBG
		DBG("naptr_iterate: choosed NAPTR rr %.*s, proto %d"
					" tried: 0x%x\n", naptr_saved->repl_len, 
					naptr_saved->repl, (int)saved_proto, *tried);
#endif
		*tried|=1<<idx;
		*proto=saved_proto;
		srv_name->s=naptr_saved->repl;
		srv_name->len=naptr_saved->repl_len;
		return l_saved;
	}
end:
	return 0;
}




/* internal sip naptr resolver function: resolves a host name trying:
 * - NAPTR lookup if the address is not an ip and *proto==0 and *port==0.
 *   The result of the NAPTR query will be used for a SRV lookup
 * - SRV lookup if the address is not an ip *port==0. The result of the SRV
 *   query will be used for an A/AAAA lookup.
 *  - normal A/AAAA lookup (either fallback from the above or if *port!=0
 *   and *proto!=0 or port==0 && proto==0)
 * when performing SRV lookup (*port==0) it will use proto to look for
 * tcp or udp hosts, otherwise proto is unused; if proto==0 => no SRV lookup
 * returns: hostent struct & *port filled with the port from the SRV record;
 *  0 on error
 */
struct hostent* naptr_sip_resolvehost(str* name,  unsigned short* port,
										char* proto)
{
	struct hostent* he;
	struct ip_addr* ip;
	static char tmp[MAX_DNS_NAME]; /* tmp. buff. for SRV lookups and
	                                  null. term  strings */
	struct rdata* l;
	struct rdata* naptr_head;
	char n_proto;
	str srv_name;
	naptr_bmp_t tried_bmp; /* tried bitmap */



	naptr_head=0;
	he=0;
	if (name->len >= MAX_DNS_NAME) {
		LOG(L_ERR, "naptr_sip_resolvehost: domain name too long\n");
		goto end;
	}
	/* try NAPTR if no port or protocol is specified and NAPTR lookup is
	 * enabled */
	if (port && proto && (*proto==0) && (*port==0)){
		*proto=PROTO_UDP; /* just in case we don't find another */
		if ( ((ip=str2ip(name))!=0)
#ifdef	USE_IPV6
			  || ((ip=str2ip6(name))!=0)
#endif
		){
			/* we are lucky, this is an ip address */
			he=ip_addr2he(name,ip);
			*port=SIP_PORT;
			goto end;
		}
		memcpy(tmp, name->s, name->len);
		tmp[name->len] = '\0';
		naptr_head=get_record(tmp, T_NAPTR, RES_AR);
		naptr_iterate_init(&tried_bmp);
		while((l=naptr_sip_iterate(naptr_head, &tried_bmp,
										&srv_name, &n_proto))!=0){
			if ((he=srv_sip_resolvehost(&srv_name, 1, port, proto, 1, l))!=0){
				*proto=n_proto;
				return he;
			}
		}
		/*clean up on exit*/
#ifdef RESOLVE_DBG
		DBG("naptr_sip_resolvehost: no NAPTR record found for %.*s," 
				" trying SRV lookup...\n", name->len, name->s);
#endif
	}
	/* fallback to normal srv lookup */
	he=srv_sip_resolvehost(name, 0, port, proto, 0, 0);
end:
	if (naptr_head)
		free_rdata_list(naptr_head);
	return he;
}
#endif /* USE_NAPTR */



/* resolves a host name trying:
 * - NAPTR lookup if enabled, the address is not an ip and *proto==0 and 
 *   *port==0. The result of the NAPTR query will be used for a SRV lookup
 * - SRV lookup if the address is not an ip *port==0. The result of the SRV
 *   query will be used for an A/AAAA lookup.
 *  - normal A/AAAA lookup (either fallback from the above or if *port!=0
 *   and *proto!=0 or port==0 && proto==0)
 * when performing SRV lookup (*port==0) it will use *proto to look for
 * tcp or udp hosts, otherwise proto is unused; if proto==0 => no SRV lookup
 *
 * returns: hostent struct & *port filled with the port from the SRV record;
 *  0 on error
 */
struct hostent* _sip_resolvehost(str* name, unsigned short* port, char* proto)
{
#ifdef USE_NAPTR
	if (dns_try_naptr)
		return naptr_sip_resolvehost(name, port, proto);
#endif
	return srv_sip_resolvehost(name, 0, port, proto, 0, 0);
}


/* resolve host, port, proto using sip rules (e.g. use SRV if port=0 a.s.o)
 *  and write the result in the sockaddr_union to
 *  returns -1 on error (resolve failed), 0 on success */
int sip_hostport2su(union sockaddr_union* su, str* name, unsigned short port,
						char* proto)
{
	struct hostent* he;
	
	he=sip_resolvehost(name, &port, proto);
	if (he==0){
		ser_error=E_BAD_ADDRESS;
		LOG(L_ERR, "ERROR: sip_hostport2su: could not resolve hostname:"
					" \"%.*s\"\n", name->len, name->s);
		goto error;
	}
	/* port filled by sip_resolvehost if empty*/
	if (hostent2su(su, he, 0, port)<0){
		ser_error=E_BAD_ADDRESS;
		goto error;
	}
	return 0;
error:
	return -1;
}
