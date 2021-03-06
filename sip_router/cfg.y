/*
 * $Id: cfg.y,v 1.185 2009/07/17 16:26:23 andrei Exp $
 *
 *  cfg grammar
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
 * ---------
 * 2003-01-29  src_port added (jiri)
 * 2003-01-23  mhomed added (jiri)
 * 2003-03-19  replaced all mallocs/frees with pkg_malloc/pkg_free (andrei)
 * 2003-03-19  Added support for route type in find_export (janakj)
 * 2003-03-20  Regex support in modparam (janakj)
 * 2003-04-01  added dst_port, proto , af (andrei)
 * 2003-04-05  s/reply_route/failure_route, onreply_route introduced (jiri)
 * 2003-04-12  added force_rport, chroot and wdir (andrei)
 * 2003-04-15  added tcp_children, disable_tcp (andrei)
 * 2003-04-22  strip_tail added (jiri)
 * 2003-07-03  tls* (disable, certificate, private_key, ca_list, verify,
 *              require_certificate added (andrei)
 * 2003-07-06  more tls config. vars added: tls_method, tls_port_no (andrei)
 * 2003-10-02  added {,set_}advertised_{address,port} (andrei)
 * 2003-10-10  added <,>,<=,>=, != operators support
 *             added msg:len (andrei)
 * 2003-10-11  if(){} doesn't require a ';' after it anymore (andrei)
 * 2003-10-13  added FIFO_DIR & proto:host:port listen/alias support (andrei)
 * 2003-10-24  converted to the new socket_info lists (andrei)
 * 2003-10-28  added tcp_accept_aliases (andrei)
 * 2003-11-20  added {tcp_connect, tcp_send, tls_*}_timeout (andrei)
 * 2004-03-30  added DISABLE_CORE and OPEN_FD_LIMIT (andrei)
 * 2004-04-29  added SOCK_MODE, SOCK_USER & SOCK_GROUP (andrei)
 * 2004-05-03  applied multicast support patch (MCAST_LOOPBACK) from janakj
 *             added MCAST_TTL (andrei)
 * 2004-07-05  src_ip & dst_ip will detect ip addresses between quotes
 *              (andrei)
 * 2004-10-19  added FROM_URI, TO_URI (andrei)
 * 2004-11-30  added force_send_socket (andrei)
 * 2005-07-08  added TCP_CON_LIFETIME, TCP_POLL_METHOD, TCP_MAX_CONNECTIONS
 *              (andrei)
 * 2005-07-11 added DNS_RETR_TIME, DNS_RETR_NO, DNS_SERVERS_NO, DNS_USE_SEARCH,
 *             DNS_TRY_IPV6 (andrei)
 * 2005-07-12  default onreply route added (andrei)
 * 2005-11-16  fixed if (cond) cmd; (andrei)
 * 2005-12-11  added onsend_route support, fcmd (filtered cmd),
 *             snd_{ip,port,proto,af}, to_{ip,proto} (andrei)
 * 2005-12-19  select framework (mma)
 * 2006-01-06  AVP index support (mma)
 * 2005-01-07  optional semicolon in statement, PARAM_STR&PARAM_STRING
 * 2006-02-02  named flags support (andrei)
 * 2006-02-06  named routes support (andrei)
 * 2006-05-30  avp flags (tma)
 * 2006-09-11  added dns cache (use, flags, ttls, mem ,gc) & dst blacklist
 *              options (andrei)
 * 2006-10-13  added STUN_ALLOW_STUN, STUN_ALLOW_FP, STUN_REFRESH_INTERVAL
 *              (vlada)
 * 2007-02-09  separated command needed for tls-in-core and for tls in general
 *              (andrei)
 * 2007-06-07  added SHM_FORCE_ALLOC, MLOCK_PAGES, REAL_TIME, RT_PRIO,
 *              RT_POLICY, RT_TIMER1_PRIO, RT_TIMER1_POLICY, RT_TIMER2_PRIO,
 *              RT_TIMER2_POLICY (andrei)
 * 2007-06-16  added DDNS_SRV_LB, DNS_TRY_NAPTR (andrei)
 * 2007-09-10  introduced phone2tel option which allows NOT to consider
 *             user=phone URIs as TEL URIs (jiri)
 * 2007-10-10  added DNS_SEARCH_FMATCH (mma)
 * 2007-11-28  added TCP_OPT_{FD_CACHE, DEFER_ACCEPT, DELAYED_ACK, SYNCNT,
 *              LINGER2, KEEPALIVE, KEEPIDLE, KEEPINTVL, KEEPCNT} (andrei)
 * 2008-01-24  added cfg_var definition (Miklos)
 * 2009-03-10  added SET_USERPHONE action (Miklos)
*/

%{

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "route_struct.h"
#include "globals.h"
#include "route.h"
#include "dprint.h"
#include "sr_module.h"
#include "modparam.h"
#include "ip_addr.h"
#include "resolve.h"
#include "socket_info.h"
#include "name_alias.h"
#include "ut.h"
#include "dset.h"
#include "select.h"
#include "flags.h"
#include "tcp_init.h"
#include "tcp_options.h"
#include "sctp_options.h"
#include "msg_translator.h"

#include "config.h"
#include "cfg_core.h"
#include "cfg/cfg.h"
#ifdef CORE_TLS
#include "tls/tls_config.h"
#endif
#include "timer_ticks.h"

#ifdef DEBUG_DMALLOC
#include <dmalloc.h>
#endif

/* hack to avoid alloca usage in the generated C file (needed for compiler
 with no built in alloca, like icc*/
#undef _ALLOCA_H

#define onsend_check(s) \
	do{\
		if (rt!=ONSEND_ROUTE) yyerror( s " allowed only in onsend_routes");\
	}while(0)


#ifdef USE_DNS_CACHE
	#define IF_DNS_CACHE(x) x
#else
	#define IF_DNS_CACHE(x) warn("dns cache support not compiled in")
#endif

#ifdef USE_DNS_FAILOVER
	#define IF_DNS_FAILOVER(x) x
#else
	#define IF_DNS_FAILOVER(x) warn("dns failover support not compiled in")
#endif

#ifdef USE_NAPTR
	#define IF_NAPTR(x) x
#else
	#define IF_NAPTR(x) warn("dns naptr support not compiled in")
#endif

#ifdef USE_DST_BLACKLIST
	#define IF_DST_BLACKLIST(x) x
#else
	#define IF_DST_BLACKLIST(x) warn("dst blacklist support not compiled in")
#endif

#ifdef USE_STUN
	#define IF_STUN(x) x
#else 
	#define IF_STUN(x) warn("stun support not compiled in")
#endif

#ifdef USE_SCTP
	#define IF_SCTP(x) x
#else
	#define IF_SCTP(x) warn("sctp support not compiled in")
#endif


extern int yylex();
/* safer then using yytext which can be array or pointer */
extern char* yy_number_str;

static void yyerror(char* s);
static char* tmp;
static int i_tmp;
static struct socket_id* lst_tmp;
static struct name_lst*  nl_tmp;
static int rt;  /* Type of route block for find_export */
static str* str_tmp;
static str s_tmp;
static struct ip_addr* ip_tmp;
static struct avp_spec* s_attr;
static select_t sel;
static select_t* sel_ptr;
static struct action *mod_func_action;

static void warn(char* s);
static struct socket_id* mk_listen_id(char*, int, int);
static struct name_lst* mk_name_lst(char* name, int flags);
static struct socket_id* mk_listen_id2(struct name_lst*, int, int);
static void free_name_lst(struct name_lst* lst);
static void free_socket_id_lst(struct socket_id* i);

%}

%union {
	long intval;
	unsigned long uval;
	char* strval;
	struct expr* expr;
	struct action* action;
	struct net* ipnet;
	struct ip_addr* ipaddr;
	struct socket_id* sockid;
	struct name_lst* name_l;
	struct avp_spec* attr;
	select_t* select;
}

/* terminals */


/* keywords */
%token FORWARD
%token FORWARD_TCP
%token FORWARD_TLS
%token FORWARD_SCTP
%token FORWARD_UDP
%token SEND
%token SEND_TCP
%token DROP
%token RETURN
%token BREAK
%token LOG_TOK
%token ERROR
%token ROUTE
%token ROUTE_FAILURE
%token ROUTE_ONREPLY
%token ROUTE_BRANCH
%token ROUTE_SEND
%token EXEC
%token SET_HOST
%token SET_HOSTPORT
%token SET_HOSTPORTTRANS
%token PREFIX
%token STRIP
%token STRIP_TAIL
%token SET_USERPHONE
%token APPEND_BRANCH
%token SET_USER
%token SET_USERPASS
%token SET_PORT
%token SET_URI
%token REVERT_URI
%token FORCE_RPORT
%token FORCE_TCP_ALIAS
%token UDP_MTU
%token UDP_MTU_TRY_PROTO
%token IF
%token ELSE
%token SET_ADV_ADDRESS
%token SET_ADV_PORT
%token FORCE_SEND_SOCKET
%token URIHOST
%token URIPORT
%token MAX_LEN
%token SETFLAG
%token RESETFLAG
%token ISFLAGSET
%token SETAVPFLAG
%token RESETAVPFLAG
%token ISAVPFLAGSET
%token METHOD
%token URI
%token FROM_URI
%token TO_URI
%token SRCIP
%token SRCPORT
%token DSTIP
%token DSTPORT
%token TOIP
%token TOPORT
%token SNDIP
%token SNDPORT
%token SNDPROTO
%token SNDAF
%token PROTO
%token AF
%token MYSELF
%token MSGLEN
%token RETCODE
%token UDP
%token TCP
%token TLS
%token SCTP

/* config vars. */
%token DEBUG_V
%token FORK
%token LOGSTDERROR
%token LOGFACILITY
%token LISTEN
%token ALIAS
%token DNS
%token REV_DNS
%token DNS_TRY_IPV6
%token DNS_TRY_NAPTR
%token DNS_SRV_LB
%token DNS_UDP_PREF
%token DNS_TCP_PREF
%token DNS_TLS_PREF
%token DNS_SCTP_PREF
%token DNS_RETR_TIME
%token DNS_RETR_NO
%token DNS_SERVERS_NO
%token DNS_USE_SEARCH
%token DNS_SEARCH_FMATCH
%token DNS_CACHE_INIT
%token DNS_USE_CACHE
%token DNS_USE_FAILOVER
%token DNS_CACHE_FLAGS
%token DNS_CACHE_NEG_TTL
%token DNS_CACHE_MIN_TTL
%token DNS_CACHE_MAX_TTL
%token DNS_CACHE_MEM
%token DNS_CACHE_GC_INT
%token DNS_CACHE_DEL_NONEXP
/*blacklist*/
%token DST_BLST_INIT
%token USE_DST_BLST
%token DST_BLST_MEM
%token DST_BLST_TTL
%token DST_BLST_GC_INT

%token PORT
%token STAT
%token CHILDREN
%token CHECK_VIA
%token PHONE2TEL
%token SYN_BRANCH
%token MEMLOG
%token MEMDBG
%token SIP_WARNING
%token SERVER_SIGNATURE
%token REPLY_TO_VIA
%token LOADMODULE
%token LOADPATH
%token MODPARAM
%token MAXBUFFER
%token USER
%token GROUP
%token CHROOT
%token WDIR
%token MHOMED
%token DISABLE_TCP
%token TCP_ACCEPT_ALIASES
%token TCP_CHILDREN
%token TCP_CONNECT_TIMEOUT
%token TCP_SEND_TIMEOUT
%token TCP_CON_LIFETIME
%token TCP_POLL_METHOD
%token TCP_MAX_CONNECTIONS
%token TCP_NO_CONNECT
%token TCP_SOURCE_IPV4
%token TCP_SOURCE_IPV6
%token TCP_OPT_FD_CACHE
%token TCP_OPT_BUF_WRITE
%token TCP_OPT_CONN_WQ_MAX
%token TCP_OPT_WQ_MAX
%token TCP_OPT_RD_BUF
%token TCP_OPT_WQ_BLK
%token TCP_OPT_DEFER_ACCEPT
%token TCP_OPT_DELAYED_ACK
%token TCP_OPT_SYNCNT
%token TCP_OPT_LINGER2
%token TCP_OPT_KEEPALIVE
%token TCP_OPT_KEEPIDLE
%token TCP_OPT_KEEPINTVL
%token TCP_OPT_KEEPCNT
%token TCP_OPT_CRLF_PING
%token DISABLE_TLS
%token ENABLE_TLS
%token TLSLOG
%token TLS_PORT_NO
%token TLS_METHOD
%token TLS_HANDSHAKE_TIMEOUT
%token TLS_SEND_TIMEOUT
%token SSLv23
%token SSLv2
%token SSLv3
%token TLSv1
%token TLS_VERIFY
%token TLS_REQUIRE_CERTIFICATE
%token TLS_CERTIFICATE
%token TLS_PRIVATE_KEY
%token TLS_CA_LIST
%token DISABLE_SCTP
%token ENABLE_SCTP
%token SCTP_CHILDREN
%token SCTP_SOCKET_RCVBUF
%token SCTP_SOCKET_SNDBUF
%token SCTP_AUTOCLOSE
%token SCTP_SEND_TTL
%token SCTP_SEND_RETRIES
%token SCTP_ASSOC_TRACKING
%token SCTP_ASSOC_REUSE
%token SCTP_MAX_ASSOCS
%token SCTP_SRTO_INITIAL
%token SCTP_SRTO_MAX
%token SCTP_SRTO_MIN
%token SCTP_ASOCMAXRXT
%token SCTP_INIT_MAX_ATTEMPTS
%token SCTP_INIT_MAX_TIMEO
%token SCTP_HBINTERVAL
%token SCTP_PATHMAXRXT
%token SCTP_SACK_DELAY
%token SCTP_SACK_FREQ
%token SCTP_MAX_BURST
%token ADVERTISED_ADDRESS
%token ADVERTISED_PORT
%token DISABLE_CORE
%token OPEN_FD_LIMIT
%token SHM_FORCE_ALLOC
%token MLOCK_PAGES
%token REAL_TIME
%token RT_PRIO
%token RT_POLICY
%token RT_TIMER1_PRIO
%token RT_TIMER1_POLICY
%token RT_TIMER2_PRIO
%token RT_TIMER2_POLICY
%token MCAST_LOOPBACK
%token MCAST_TTL
%token TOS
%token PMTU_DISCOVERY
%token KILL_TIMEOUT
%token CFG_DESCRIPTION
%token SERVER_ID

%token FLAGS_DECL
%token AVPFLAGS_DECL

%token ATTR_MARK
%token SELECT_MARK
%token ATTR_FROM
%token ATTR_TO
%token ATTR_FROMURI
%token ATTR_TOURI
%token ATTR_FROMUSER
%token ATTR_TOUSER
%token ATTR_FROMDOMAIN
%token ATTR_TODOMAIN
%token ATTR_GLOBAL
%token ADDEQ

%token STUN_REFRESH_INTERVAL
%token STUN_ALLOW_STUN
%token STUN_ALLOW_FP


/* operators */
%nonassoc EQUAL
%nonassoc EQUAL_T
%nonassoc GT
%nonassoc LT
%nonassoc GTE
%nonassoc LTE
%nonassoc DIFF
%nonassoc MATCH
%left LOG_OR
%left LOG_AND
%left BIN_OR
%left BIN_AND
%left PLUS MINUS
%right NOT

/* values */
%token <intval> NUMBER
%token <strval> ID
%token <strval> NUM_ID
%token <strval> STRING
%token <strval> IPV6ADDR

/* other */
%token COMMA
%token SEMICOLON
%token RPAREN
%token LPAREN
%token LBRACE
%token RBRACE
%token LBRACK
%token RBRACK
%token SLASH
%token DOT
%token CR
%token COLON
%token STAR


/*non-terminals */
%type <expr> exp exp_elem /*, condition*/
%type <action> action actions cmd fcmd if_cmd stm exp_stm assign_action
%type <ipaddr> ipv4 ipv6 ipv6addr ip
%type <ipnet> ipnet
%type <strval> host host_or_if host_if_id
%type <strval> listen_id
%type <name_l> listen_id_lst
%type <name_l> listen_id2
%type <sockid>  id_lst
%type <sockid>  phostport
%type <sockid>  listen_phostport
%type <intval> proto port
%type <intval> equalop strop intop binop
%type <strval> host_sep
%type <intval> uri_type
%type <attr> attr_id
%type <attr> attr_id_num_idx
%type <attr> attr_id_no_idx
%type <attr> attr_id_ass
%type <attr> attr_id_val
%type <attr> attr_id_any
%type <attr> attr_id_any_str
/* %type <intval> class_id */
%type <intval> assign_op
%type <select> select_id
%type <strval>	flag_name;
%type <strval>	route_name;
%type <intval> avpflag_oper

/*%type <route_el> rules;
  %type <route_el> rule;
*/

%%


cfg:
	statements
	;
statements:
	statements statement {}
	| statement {}
	| statements error { yyerror(""); YYABORT;}
	;
statement:
	assign_stm
	| flags_decl
	| avpflags_decl
	| module_stm
	| {rt=REQUEST_ROUTE;} route_stm
	| {rt=FAILURE_ROUTE;} failure_route_stm
	| {rt=ONREPLY_ROUTE;} onreply_route_stm
	| {rt=BRANCH_ROUTE;} branch_route_stm
	| {rt=ONSEND_ROUTE;}   send_route_stm
	| SEMICOLON	/* null statement */
	| CR	/* null statement*/
	;
listen_id:
	ip {
		if ($1){
			tmp=ip_addr2a($1);
			if (tmp==0) {
				LOG(L_CRIT, "ERROR: cfg. parser: bad ip "
						"address.\n");
				$$=0;
			} else {
				$$=pkg_malloc(strlen(tmp)+1);
				if ($$==0) {
					LOG(L_CRIT, "ERROR: cfg. parser: out of "
							"memory.\n");
				} else {
					strncpy($$, tmp, strlen(tmp)+1);
				}
			}
		}
	}
	| STRING {
		$$=pkg_malloc(strlen($1)+1);
		if ($$==0) {
				LOG(L_CRIT, "ERROR: cfg. parser: out of "
						"memory.\n");
		} else {
				strncpy($$, $1, strlen($1)+1);
		}
	}
	| host_or_if {
		if ($1){
			$$=pkg_malloc(strlen($1)+1);
			if ($$==0) {
					LOG(L_CRIT, "ERROR: cfg. parser: out of "
							"memory.\n");
			} else {
					strncpy($$, $1, strlen($1)+1);
			}
		}
	}
	;


listen_id_lst:
	listen_id	{ $$=mk_name_lst($1, SI_IS_MHOMED); }
	| listen_id COMMA listen_id_lst	{ $$=mk_name_lst($1, SI_IS_MHOMED); 
										if ($$) $$->next=$3;
									}
	;

listen_id2:
	LPAREN listen_id_lst RPAREN { $$=$2; }
	| listen_id	{ $$=mk_name_lst($1, 0); }
	;

proto:
	UDP	{ $$=PROTO_UDP; }
	| TCP	{ $$=PROTO_TCP; }
	| TLS	{ $$=PROTO_TLS; }
	| SCTP	{ $$=PROTO_SCTP; }
	| STAR	{ $$=0; }
	;
port:
	NUMBER	{ $$=$1; }
	| STAR	{ $$=0; }
;
phostport:
	listen_id		{ $$=mk_listen_id($1, 0, 0); }
	| listen_id COLON port	{ $$=mk_listen_id($1, 0, $3); }
	| proto COLON listen_id	{ $$=mk_listen_id($3, $1, 0); }
	| proto COLON listen_id COLON port	{ $$=mk_listen_id($3, $1, $5);}
	| listen_id COLON error { $$=0; yyerror(" port number expected"); }
	;

listen_phostport:
	listen_id2		{ $$=mk_listen_id2($1, 0, 0); }
	| listen_id2 COLON port	{ $$=mk_listen_id2($1, 0, $3); }
	| proto COLON listen_id2	{ $$=mk_listen_id2($3, $1, 0); }
	| proto COLON listen_id2 COLON port	{ $$=mk_listen_id2($3, $1, $5);}
	| listen_id2 COLON error { $$=0; yyerror(" port number expected"); }
	;

id_lst:
	listen_phostport		{  $$=$1 ; }
	| listen_phostport id_lst	{ $$=$1;  if ($$) $$->next=$2; }
	;

flags_decl:		FLAGS_DECL	flag_list
			|	FLAGS_DECL error { yyerror("flag list expected\n"); }
;
flag_list:		flag_spec
			|	flag_spec COMMA flag_list
;

flag_spec:		flag_name	{ if (register_flag($1,-1)<0)
								yyerror("register flag failed");
						}
			|	flag_name COLON NUMBER {
						if (register_flag($1, $3)<0)
								yyerror("register flag failed");
										}
;

flag_name:		STRING	{ $$=$1; }
			|	ID		{ $$=$1; }
;

avpflags_decl:
	AVPFLAGS_DECL avpflag_list
	| AVPFLAGS_DECL error { yyerror("avpflag list expected\n"); }
	;
avpflag_list:
	avpflag_spec
	| avpflag_spec COMMA avpflag_list
	;
avpflag_spec:
	flag_name {
		if (register_avpflag($1)==0)
			yyerror("cannot declare avpflag");
	}
	;
assign_stm:
	DEBUG_V EQUAL NUMBER { default_core_cfg.debug=$3; }
	| DEBUG_V EQUAL error  { yyerror("number  expected"); }
	| FORK  EQUAL NUMBER { dont_fork= ! $3; }
	| FORK  EQUAL error  { yyerror("boolean value expected"); }
	| LOGSTDERROR EQUAL NUMBER { if (!config_check) log_stderr=$3; }
	| LOGSTDERROR EQUAL error { yyerror("boolean value expected"); }
	| LOGFACILITY EQUAL ID {
		if ( (i_tmp=str2facility($3))==-1)
			yyerror("bad facility (see syslog(3) man page)");
		if (!config_check)
			default_core_cfg.log_facility=i_tmp;
	}
	| LOGFACILITY EQUAL error { yyerror("ID expected"); }
	| DNS EQUAL NUMBER   { received_dns|= ($3)?DO_DNS:0; }
	| DNS EQUAL error { yyerror("boolean value expected"); }
	| REV_DNS EQUAL NUMBER { received_dns|= ($3)?DO_REV_DNS:0; }
	| REV_DNS EQUAL error { yyerror("boolean value expected"); }
	| DNS_TRY_IPV6 EQUAL NUMBER   { default_core_cfg.dns_try_ipv6=$3; }
	| DNS_TRY_IPV6 error { yyerror("boolean value expected"); }
	| DNS_TRY_NAPTR EQUAL NUMBER   { IF_NAPTR(default_core_cfg.dns_try_naptr=$3); }
	| DNS_TRY_NAPTR error { yyerror("boolean value expected"); }
	| DNS_SRV_LB EQUAL NUMBER   { IF_DNS_FAILOVER(default_core_cfg.dns_srv_lb=$3); }
	| DNS_SRV_LB error { yyerror("boolean value expected"); }
	| DNS_UDP_PREF EQUAL NUMBER { IF_NAPTR(default_core_cfg.dns_udp_pref=$3);}
	| DNS_UDP_PREF error { yyerror("number expected"); }
	| DNS_TCP_PREF EQUAL NUMBER { IF_NAPTR(default_core_cfg.dns_tcp_pref=$3);}
	| DNS_TCP_PREF error { yyerror("number expected"); }
	| DNS_TLS_PREF EQUAL NUMBER { IF_NAPTR(default_core_cfg.dns_tls_pref=$3);}
	| DNS_TLS_PREF error { yyerror("number expected"); }
	| DNS_SCTP_PREF EQUAL NUMBER { 
								IF_NAPTR(default_core_cfg.dns_sctp_pref=$3); }
	| DNS_SCTP_PREF error { yyerror("number expected"); }
	| DNS_RETR_TIME EQUAL NUMBER   { default_core_cfg.dns_retr_time=$3; }
	| DNS_RETR_TIME error { yyerror("number expected"); }
	| DNS_RETR_NO EQUAL NUMBER   { default_core_cfg.dns_retr_no=$3; }
	| DNS_RETR_NO error { yyerror("number expected"); }
	| DNS_SERVERS_NO EQUAL NUMBER   { default_core_cfg.dns_servers_no=$3; }
	| DNS_SERVERS_NO error { yyerror("number expected"); }
	| DNS_USE_SEARCH EQUAL NUMBER   { default_core_cfg.dns_search_list=$3; }
	| DNS_USE_SEARCH error { yyerror("boolean value expected"); }
	| DNS_SEARCH_FMATCH EQUAL NUMBER   { default_core_cfg.dns_search_fmatch=$3; }
	| DNS_SEARCH_FMATCH error { yyerror("boolean value expected"); }
	| DNS_CACHE_INIT EQUAL NUMBER   { IF_DNS_CACHE(dns_cache_init=$3); }
	| DNS_CACHE_INIT error { yyerror("boolean value expected"); }
	| DNS_USE_CACHE EQUAL NUMBER   { IF_DNS_CACHE(default_core_cfg.use_dns_cache=$3); }
	| DNS_USE_CACHE error { yyerror("boolean value expected"); }
	| DNS_USE_FAILOVER EQUAL NUMBER   { IF_DNS_FAILOVER(default_core_cfg.use_dns_failover=$3);}
	| DNS_USE_FAILOVER error { yyerror("boolean value expected"); }
	| DNS_CACHE_FLAGS EQUAL NUMBER   { IF_DNS_CACHE(default_core_cfg.dns_cache_flags=$3); }
	| DNS_CACHE_FLAGS error { yyerror("boolean value expected"); }
	| DNS_CACHE_NEG_TTL EQUAL NUMBER   { IF_DNS_CACHE(default_core_cfg.dns_neg_cache_ttl=$3); }
	| DNS_CACHE_NEG_TTL error { yyerror("boolean value expected"); }
	| DNS_CACHE_MAX_TTL EQUAL NUMBER   { IF_DNS_CACHE(default_core_cfg.dns_cache_max_ttl=$3); }
	| DNS_CACHE_MAX_TTL error { yyerror("boolean value expected"); }
	| DNS_CACHE_MIN_TTL EQUAL NUMBER   { IF_DNS_CACHE(default_core_cfg.dns_cache_min_ttl=$3); }
	| DNS_CACHE_MIN_TTL error { yyerror("boolean value expected"); }
	| DNS_CACHE_MEM EQUAL NUMBER   { IF_DNS_CACHE(default_core_cfg.dns_cache_max_mem=$3); }
	| DNS_CACHE_MEM error { yyerror("boolean value expected"); }
	| DNS_CACHE_GC_INT EQUAL NUMBER   { IF_DNS_CACHE(dns_timer_interval=$3); }
	| DNS_CACHE_GC_INT error { yyerror("boolean value expected"); }
	| DNS_CACHE_DEL_NONEXP EQUAL NUMBER   { IF_DNS_CACHE(default_core_cfg.dns_cache_del_nonexp=$3); }
	| DNS_CACHE_DEL_NONEXP error { yyerror("boolean value expected"); }
	| DST_BLST_INIT EQUAL NUMBER   { IF_DST_BLACKLIST(dst_blacklist_init=$3); }
	| DST_BLST_INIT error { yyerror("boolean value expected"); }
	| USE_DST_BLST EQUAL NUMBER   { IF_DST_BLACKLIST(default_core_cfg.use_dst_blacklist=$3); }
	| USE_DST_BLST error { yyerror("boolean value expected"); }
	| DST_BLST_MEM EQUAL NUMBER   { IF_DST_BLACKLIST(default_core_cfg.blst_max_mem=$3); }
	| DST_BLST_MEM error { yyerror("boolean value expected"); }
	| DST_BLST_TTL EQUAL NUMBER   { IF_DST_BLACKLIST(default_core_cfg.blst_timeout=$3); }
	| DST_BLST_TTL error { yyerror("boolean value expected"); }
	| DST_BLST_GC_INT EQUAL NUMBER { IF_DST_BLACKLIST(blst_timer_interval=$3);}
	| DST_BLST_GC_INT error { yyerror("boolean value expected"); }
	| PORT EQUAL NUMBER   { port_no=$3; }
	| STAT EQUAL STRING {
		#ifdef STATS
				stat_file=$3;
		#endif
	}
	| MAXBUFFER EQUAL NUMBER { maxbuffer=$3; }
	| MAXBUFFER EQUAL error { yyerror("number expected"); }
	| PORT EQUAL error    { yyerror("number expected"); }
	| CHILDREN EQUAL NUMBER { children_no=$3; }
	| CHILDREN EQUAL error { yyerror("number expected"); }
	| CHECK_VIA EQUAL NUMBER { check_via=$3; }
	| CHECK_VIA EQUAL error { yyerror("boolean value expected"); }
	| PHONE2TEL EQUAL NUMBER { phone2tel=$3; }
	| PHONE2TEL EQUAL error { yyerror("boolean value expected"); }
	| SYN_BRANCH EQUAL NUMBER { syn_branch=$3; }
	| SYN_BRANCH EQUAL error { yyerror("boolean value expected"); }
	| MEMLOG EQUAL NUMBER { memlog=$3; }
	| MEMLOG EQUAL error { yyerror("int value expected"); }
	| MEMDBG EQUAL NUMBER { memdbg=$3; }
	| MEMDBG EQUAL error { yyerror("int value expected"); }
	| SIP_WARNING EQUAL NUMBER { sip_warning=$3; }
	| SIP_WARNING EQUAL error { yyerror("boolean value expected"); }
	| USER EQUAL STRING     { user=$3; }
	| USER EQUAL ID         { user=$3; }
	| USER EQUAL error      { yyerror("string value expected"); }
	| GROUP EQUAL STRING     { group=$3; }
	| GROUP EQUAL ID         { group=$3; }
	| GROUP EQUAL error      { yyerror("string value expected"); }
	| CHROOT EQUAL STRING     { chroot_dir=$3; }
	| CHROOT EQUAL ID         { chroot_dir=$3; }
	| CHROOT EQUAL error      { yyerror("string value expected"); }
	| WDIR EQUAL STRING     { working_dir=$3; }
	| WDIR EQUAL ID         { working_dir=$3; }
	| WDIR EQUAL error      { yyerror("string value expected"); }
	| MHOMED EQUAL NUMBER { mhomed=$3; }
	| MHOMED EQUAL error { yyerror("boolean value expected"); }
	| DISABLE_TCP EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_disable=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| DISABLE_TCP EQUAL error { yyerror("boolean value expected"); }
	| TCP_ACCEPT_ALIASES EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.accept_aliases=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_ACCEPT_ALIASES EQUAL error { yyerror("boolean value expected"); }
	| TCP_CHILDREN EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_children_no=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_CHILDREN EQUAL error { yyerror("number expected"); }
	| TCP_CONNECT_TIMEOUT EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.connect_timeout_s=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_CONNECT_TIMEOUT EQUAL error { yyerror("number expected"); }
	| TCP_SEND_TIMEOUT EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.send_timeout=S_TO_TICKS($3);
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_SEND_TIMEOUT EQUAL error { yyerror("number expected"); }
	| TCP_CON_LIFETIME EQUAL NUMBER {
		#ifdef USE_TCP
			if ($3<0)
				tcp_default_cfg.con_lifetime=-1;
			else
				tcp_default_cfg.con_lifetime=S_TO_TICKS($3);
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_CON_LIFETIME EQUAL error { yyerror("number expected"); }
	| TCP_POLL_METHOD EQUAL ID {
		#ifdef USE_TCP
			tcp_poll_method=get_poll_type($3);
			if (tcp_poll_method==POLL_NONE) {
				LOG(L_CRIT, "bad poll method name:"
						" %s\n, try one of %s.\n",
						$3, poll_support);
				yyerror("bad tcp_poll_method "
						"value");
			}
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_POLL_METHOD EQUAL STRING {
		#ifdef USE_TCP
			tcp_poll_method=get_poll_type($3);
			if (tcp_poll_method==POLL_NONE) {
				LOG(L_CRIT, "bad poll method name:"
						" %s\n, try one of %s.\n",
						$3, poll_support);
				yyerror("bad tcp_poll_method "
						"value");
			}
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_POLL_METHOD EQUAL error { yyerror("poll method name expected"); }
	| TCP_MAX_CONNECTIONS EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_max_connections=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_MAX_CONNECTIONS EQUAL error { yyerror("number expected"); }
	| TCP_NO_CONNECT EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.no_connect=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_NO_CONNECT EQUAL error { yyerror("boolean value expected"); }
	| TCP_SOURCE_IPV4 EQUAL ipv4 {
		#ifdef USE_TCP
			if (tcp_set_src_addr($3)<0)
				warn("tcp_source_ipv4 failed");
		#else
			warn("tcp support not compiled in");
		#endif
		pkg_free($3);
	}
	| TCP_SOURCE_IPV4 EQUAL error { yyerror("IPv4 address expected"); }
	| TCP_SOURCE_IPV6 EQUAL ipv6 {
		#ifdef USE_TCP
			#ifdef USE_IPV6
				if (tcp_set_src_addr($3)<0)
					warn("tcp_source_ipv6 failed");
			#else
				warn("IPv6 support not compiled in");
			#endif
		#else
			warn("tcp support not compiled in");
		#endif
		pkg_free($3);
	}
	| TCP_SOURCE_IPV6 EQUAL error { yyerror("IPv6 address expected"); }
	| TCP_OPT_FD_CACHE EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.fd_cache=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_OPT_FD_CACHE EQUAL error { yyerror("boolean value expected"); }
	| TCP_OPT_BUF_WRITE EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.async=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_OPT_BUF_WRITE EQUAL error { yyerror("boolean value expected"); }
	| TCP_OPT_CONN_WQ_MAX EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.tcpconn_wq_max=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_OPT_CONN_WQ_MAX error { yyerror("boolean value expected"); }
	| TCP_OPT_WQ_MAX EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.tcp_wq_max=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_OPT_WQ_MAX error { yyerror("number expected"); }
	| TCP_OPT_RD_BUF EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.rd_buf_size=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_OPT_RD_BUF error { yyerror("number expected"); }
	| TCP_OPT_WQ_BLK EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.wq_blk_size=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_OPT_WQ_BLK error { yyerror("number expected"); }
	| TCP_OPT_DEFER_ACCEPT EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.defer_accept=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_OPT_DEFER_ACCEPT EQUAL error { yyerror("boolean value expected"); }
	| TCP_OPT_DELAYED_ACK EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.delayed_ack=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_OPT_DELAYED_ACK EQUAL error { yyerror("boolean value expected"); }
	| TCP_OPT_SYNCNT EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.syncnt=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_OPT_SYNCNT EQUAL error { yyerror("number expected"); }
	| TCP_OPT_LINGER2 EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.linger2=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_OPT_LINGER2 EQUAL error { yyerror("number expected"); }
	| TCP_OPT_KEEPALIVE EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.keepalive=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_OPT_KEEPALIVE EQUAL error { yyerror("boolean value expected");}
	| TCP_OPT_KEEPIDLE EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.keepidle=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_OPT_KEEPIDLE EQUAL error { yyerror("number expected"); }
	| TCP_OPT_KEEPINTVL EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.keepintvl=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_OPT_KEEPINTVL EQUAL error { yyerror("number expected"); }
	| TCP_OPT_KEEPCNT EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.keepcnt=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_OPT_KEEPCNT EQUAL error { yyerror("number expected"); }
	| TCP_OPT_CRLF_PING EQUAL NUMBER {
		#ifdef USE_TCP
			tcp_default_cfg.crlf_ping=$3;
		#else
			warn("tcp support not compiled in");
		#endif
	}
	| TCP_OPT_CRLF_PING EQUAL error { yyerror("boolean value expected"); }
	| DISABLE_TLS EQUAL NUMBER {
		#ifdef USE_TLS
			tls_disable=$3;
		#else
			warn("tls support not compiled in");
		#endif
	}
	| DISABLE_TLS EQUAL error { yyerror("boolean value expected"); }
	| ENABLE_TLS EQUAL NUMBER {
		#ifdef USE_TLS
			tls_disable=!($3);
		#else
			warn("tls support not compiled in");
		#endif
	}
	| ENABLE_TLS EQUAL error { yyerror("boolean value expected"); }
	| TLSLOG EQUAL NUMBER {
		#ifdef CORE_TLS
			tls_log=$3;
		#else
			warn("tls-in-core support not compiled in");
		#endif
	}
	| TLSLOG EQUAL error { yyerror("int value expected"); }
	| TLS_PORT_NO EQUAL NUMBER {
		#ifdef USE_TLS
			tls_port_no=$3;
		#else
			warn("tls support not compiled in");
		#endif
	}
	| TLS_PORT_NO EQUAL error { yyerror("number expected"); }
	| TLS_METHOD EQUAL SSLv23 {
		#ifdef CORE_TLS
			tls_method=TLS_USE_SSLv23;
		#else
			warn("tls-in-core support not compiled in");
		#endif
	}
	| TLS_METHOD EQUAL SSLv2 {
		#ifdef CORE_TLS
			tls_method=TLS_USE_SSLv2;
		#else
			warn("tls-in-core support not compiled in");
		#endif
	}
	| TLS_METHOD EQUAL SSLv3 {
		#ifdef CORE_TLS
			tls_method=TLS_USE_SSLv3;
		#else
			warn("tls-in-core support not compiled in");
		#endif
	}
	| TLS_METHOD EQUAL TLSv1 {
		#ifdef CORE_TLS
			tls_method=TLS_USE_TLSv1;
		#else
			warn("tls-in-core support not compiled in");
		#endif
	}
	| TLS_METHOD EQUAL error {
		#ifdef CORE_TLS
			yyerror("SSLv23, SSLv2, SSLv3 or TLSv1 expected");
		#else
			warn("tls-in-core support not compiled in");
		#endif
	}
	| TLS_VERIFY EQUAL NUMBER {
		#ifdef CORE_TLS
			tls_verify_cert=$3;
		#else
			warn("tls-in-core support not compiled in");
		#endif
	}
	| TLS_VERIFY EQUAL error { yyerror("boolean value expected"); }
	| TLS_REQUIRE_CERTIFICATE EQUAL NUMBER {
		#ifdef CORE_TLS
			tls_require_cert=$3;
		#else
			warn( "tls-in-core support not compiled in");
		#endif
	}
	| TLS_REQUIRE_CERTIFICATE EQUAL error { yyerror("boolean value expected"); }
	| TLS_CERTIFICATE EQUAL STRING {
		#ifdef CORE_TLS
			tls_cert_file=$3;
		#else
			warn("tls-in-core support not compiled in");
		#endif
	}
	| TLS_CERTIFICATE EQUAL error { yyerror("string value expected"); }
	| TLS_PRIVATE_KEY EQUAL STRING {
		#ifdef CORE_TLS
			tls_pkey_file=$3;
		#else
			warn("tls-in-core support not compiled in");
		#endif
	}
	| TLS_PRIVATE_KEY EQUAL error { yyerror("string value expected"); }
	| TLS_CA_LIST EQUAL STRING {
		#ifdef CORE_TLS
			tls_ca_file=$3;
		#else
			warn("tls-in-core support not compiled in");
		#endif
	}
	| TLS_CA_LIST EQUAL error { yyerror("string value expected"); }
	| TLS_HANDSHAKE_TIMEOUT EQUAL NUMBER {
		#ifdef CORE_TLS
			tls_handshake_timeout=$3;
		#else
			warn("tls-in-core support not compiled in");
		#endif
	}
	| TLS_HANDSHAKE_TIMEOUT EQUAL error { yyerror("number expected"); }
	| TLS_SEND_TIMEOUT EQUAL NUMBER {
		#ifdef CORE_TLS
			tls_send_timeout=$3;
		#else
			warn("tls-in-core support not compiled in");
		#endif
	}
	| TLS_SEND_TIMEOUT EQUAL error { yyerror("number expected"); }
	| DISABLE_SCTP EQUAL NUMBER {
		#ifdef USE_SCTP
			sctp_disable=$3;
		#else
			warn("sctp support not compiled in");
		#endif
	}
	| DISABLE_SCTP EQUAL error { yyerror("boolean value expected"); }
	| ENABLE_SCTP EQUAL NUMBER {
		#ifdef USE_SCTP
			sctp_disable=($3<=1)?!$3:$3;
		#else
			warn("sctp support not compiled in");
		#endif
	}
	| ENABLE_SCTP EQUAL error { yyerror("boolean or number expected"); }
	| SCTP_CHILDREN EQUAL NUMBER {
		#ifdef USE_SCTP
			sctp_children_no=$3;
		#else
			warn("sctp support not compiled in");
		#endif
	}
	| SCTP_CHILDREN EQUAL error { yyerror("number expected"); }
	| SCTP_SOCKET_RCVBUF EQUAL NUMBER {
		#ifdef USE_SCTP
			sctp_default_cfg.so_rcvbuf=$3;
		#else
			warn("sctp support not compiled in");
		#endif
	}
	| SCTP_SOCKET_RCVBUF EQUAL error { yyerror("number expected"); }
	| SCTP_SOCKET_SNDBUF EQUAL NUMBER {
		#ifdef USE_SCTP
			sctp_default_cfg.so_sndbuf=$3;
		#else
			warn("sctp support not compiled in");
		#endif
	}
	| SCTP_SOCKET_SNDBUF EQUAL error { yyerror("number expected"); }
	| SCTP_AUTOCLOSE EQUAL NUMBER {
		#ifdef USE_SCTP
			sctp_default_cfg.autoclose=$3;
		#else
			warn("sctp support not compiled in");
		#endif
	}
	| SCTP_AUTOCLOSE EQUAL error { yyerror("number expected"); }
	| SCTP_SEND_TTL EQUAL NUMBER {
		#ifdef USE_SCTP
			sctp_default_cfg.send_ttl=$3;
		#else
			warn("sctp support not compiled in");
		#endif
	}
	| SCTP_SEND_TTL EQUAL error { yyerror("number expected"); }
	| SCTP_SEND_RETRIES EQUAL NUMBER {
		#ifdef USE_SCTP
			sctp_default_cfg.send_retries=$3;
		#else
			warn("sctp support not compiled in");
		#endif
	}
	| SCTP_SEND_RETRIES EQUAL error { yyerror("number expected"); }
	| SCTP_ASSOC_TRACKING EQUAL NUMBER {
		#ifdef USE_SCTP
			#ifdef SCTP_CONN_REUSE
				sctp_default_cfg.assoc_tracking=$3;
			#else
				if ($3)
					warn("sctp association tracking/reuse (SCTP_CONN_REUSE) "
							"support not compiled in");
			#endif /* SCTP_CONN_REUSE */
		#else
			warn("sctp support not compiled in");
		#endif /* USE_SCTP */
	}
	| SCTP_ASSOC_TRACKING EQUAL error { yyerror("number expected"); }
	| SCTP_ASSOC_REUSE EQUAL NUMBER {
		#ifdef USE_SCTP
			#ifdef SCTP_CONN_REUSE
				sctp_default_cfg.assoc_reuse=$3;
			#else
				if ($3)
					warn("sctp association reuse (SCTP_CONN_REUSE) support"
							" not compiled in");
			#endif /* SCTP_CONN_REUSE */
		#else
			warn("sctp support not compiled in");
		#endif /* USE_SCTP */
	}
	| SCTP_ASSOC_REUSE EQUAL error { yyerror("number expected"); }
	| SCTP_MAX_ASSOCS EQUAL NUMBER {
			IF_SCTP(sctp_default_cfg.max_assocs=$3);
	}
	| SCTP_MAX_ASSOCS EQUAL error { yyerror("number expected"); }
	| SCTP_SRTO_INITIAL EQUAL NUMBER {
			IF_SCTP(sctp_default_cfg.srto_initial=$3);
	}
	| SCTP_SRTO_INITIAL EQUAL error { yyerror("number expected"); }
	| SCTP_SRTO_MAX EQUAL NUMBER {
			IF_SCTP(sctp_default_cfg.srto_max=$3);
	}
	| SCTP_SRTO_MAX EQUAL error { yyerror("number expected"); }
	| SCTP_SRTO_MIN EQUAL NUMBER {
			IF_SCTP(sctp_default_cfg.srto_min=$3);
	}
	| SCTP_SRTO_MIN EQUAL error { yyerror("number expected"); }
	| SCTP_ASOCMAXRXT EQUAL NUMBER {
			IF_SCTP(sctp_default_cfg.asocmaxrxt=$3);
	}
	| SCTP_ASOCMAXRXT EQUAL error { yyerror("number expected"); }
	| SCTP_INIT_MAX_ATTEMPTS EQUAL NUMBER {
			IF_SCTP(sctp_default_cfg.init_max_attempts=$3);
	}
	| SCTP_INIT_MAX_ATTEMPTS EQUAL error { yyerror("number expected"); }
	| SCTP_INIT_MAX_TIMEO EQUAL NUMBER {
			IF_SCTP(sctp_default_cfg.init_max_timeo=$3);
	}
	| SCTP_INIT_MAX_TIMEO EQUAL error { yyerror("number expected"); }
	| SCTP_HBINTERVAL EQUAL NUMBER {
			IF_SCTP(sctp_default_cfg.hbinterval=$3);
	}
	| SCTP_HBINTERVAL EQUAL error { yyerror("number expected"); }
	| SCTP_PATHMAXRXT EQUAL NUMBER {
			IF_SCTP(sctp_default_cfg.pathmaxrxt=$3);
	}
	| SCTP_PATHMAXRXT EQUAL error { yyerror("number expected"); }
	| SCTP_SACK_DELAY EQUAL NUMBER {
			IF_SCTP(sctp_default_cfg.sack_delay=$3);
	}
	| SCTP_SACK_DELAY EQUAL error { yyerror("number expected"); }
	| SCTP_SACK_FREQ EQUAL NUMBER {
			IF_SCTP(sctp_default_cfg.sack_freq=$3);
	}
	| SCTP_SACK_FREQ EQUAL error { yyerror("number expected"); }
	| SCTP_MAX_BURST EQUAL NUMBER {
			IF_SCTP(sctp_default_cfg.max_burst=$3);
	}
	| SCTP_MAX_BURST EQUAL error { yyerror("number expected"); }
	| SERVER_SIGNATURE EQUAL NUMBER { server_signature=$3; }
	| SERVER_SIGNATURE EQUAL error { yyerror("boolean value expected"); }
	| REPLY_TO_VIA EQUAL NUMBER { reply_to_via=$3; }
	| REPLY_TO_VIA EQUAL error { yyerror("boolean value expected"); }
	| LISTEN EQUAL id_lst {
		for(lst_tmp=$3; lst_tmp; lst_tmp=lst_tmp->next) {
			if (add_listen_iface(	lst_tmp->addr_lst->name,
									lst_tmp->addr_lst->next,
									lst_tmp->port, lst_tmp->proto,
									lst_tmp->flags)!=0) {
				LOG(L_CRIT,  "ERROR: cfg. parser: failed to add listen"
								" address\n");
				break;
			}
		}
		free_socket_id_lst($3);
	}
	| LISTEN EQUAL  error { yyerror("ip address, interface name or"
									" hostname expected"); }
	| ALIAS EQUAL  id_lst {
		for(lst_tmp=$3; lst_tmp; lst_tmp=lst_tmp->next){
			add_alias(	lst_tmp->addr_lst->name,
						strlen(lst_tmp->addr_lst->name),
						lst_tmp->port, lst_tmp->proto);
			for (nl_tmp=lst_tmp->addr_lst->next; nl_tmp; nl_tmp=nl_tmp->next)
				add_alias(nl_tmp->name, strlen(nl_tmp->name),
							lst_tmp->port, lst_tmp->proto);
		}
		free_socket_id_lst($3);
	}
	| ALIAS  EQUAL error  { yyerror(" hostname expected"); }
	| ADVERTISED_ADDRESS EQUAL listen_id {
		if ($3){
			default_global_address.s=$3;
			default_global_address.len=strlen($3);
		}
	}
	| ADVERTISED_ADDRESS EQUAL error {yyerror("ip address or hostname expected"); }
	| ADVERTISED_PORT EQUAL NUMBER {
		tmp=int2str($3, &i_tmp);
		if ((default_global_port.s=pkg_malloc(i_tmp))==0) {
			LOG(L_CRIT, "ERROR: cfg. parser: out of memory.\n");
			default_global_port.len=0;
		} else {
			default_global_port.len=i_tmp;
			memcpy(default_global_port.s, tmp, default_global_port.len);
		};
	}
	|ADVERTISED_PORT EQUAL error {yyerror("ip address or hostname expected"); }
	| DISABLE_CORE EQUAL NUMBER { disable_core_dump=$3; }
	| DISABLE_CORE EQUAL error { yyerror("boolean value expected"); }
	| OPEN_FD_LIMIT EQUAL NUMBER { open_files_limit=$3; }
	| OPEN_FD_LIMIT EQUAL error { yyerror("number expected"); }
	| SHM_FORCE_ALLOC EQUAL NUMBER { shm_force_alloc=$3; }
	| SHM_FORCE_ALLOC EQUAL error { yyerror("boolean value expected"); }
	| MLOCK_PAGES EQUAL NUMBER { mlock_pages=$3; }
	| MLOCK_PAGES EQUAL error { yyerror("boolean value expected"); }
	| REAL_TIME EQUAL NUMBER { real_time=$3; }
	| REAL_TIME EQUAL error { yyerror("boolean value expected"); }
	| RT_PRIO EQUAL NUMBER { rt_prio=$3; }
	| RT_PRIO EQUAL error { yyerror("boolean value expected"); }
	| RT_POLICY EQUAL NUMBER { rt_policy=$3; }
	| RT_POLICY EQUAL error { yyerror("boolean value expected"); }
	| RT_TIMER1_PRIO EQUAL NUMBER { rt_timer1_prio=$3; }
	| RT_TIMER1_PRIO EQUAL error { yyerror("boolean value expected"); }
	| RT_TIMER1_POLICY EQUAL NUMBER { rt_timer1_policy=$3; }
	| RT_TIMER1_POLICY EQUAL error { yyerror("boolean value expected"); }
	| RT_TIMER2_PRIO EQUAL NUMBER { rt_timer2_prio=$3; }
	| RT_TIMER2_PRIO EQUAL error { yyerror("boolean value expected"); }
	| RT_TIMER2_POLICY EQUAL NUMBER { rt_timer2_policy=$3; }
	| RT_TIMER2_POLICY EQUAL error { yyerror("boolean value expected"); }
	| MCAST_LOOPBACK EQUAL NUMBER {
		#ifdef USE_MCAST
			mcast_loopback=$3;
		#else
			warn("no multicast support compiled in");
		#endif
	}
	| MCAST_LOOPBACK EQUAL error { yyerror("boolean value expected"); }
	| MCAST_TTL EQUAL NUMBER {
		#ifdef USE_MCAST
			mcast_ttl=$3;
		#else
			warn("no multicast support compiled in");
		#endif
	}
	| MCAST_TTL EQUAL error { yyerror("number expected"); }
	| TOS EQUAL NUMBER { tos=$3; }
	| TOS EQUAL error { yyerror("number expected"); }
	| PMTU_DISCOVERY EQUAL NUMBER { pmtu_discovery=$3; }
	| PMTU_DISCOVERY error { yyerror("number expected"); }
	| KILL_TIMEOUT EQUAL NUMBER { ser_kill_timeout=$3; }
	| KILL_TIMEOUT EQUAL error { yyerror("number expected"); }
	| STUN_REFRESH_INTERVAL EQUAL NUMBER { IF_STUN(stun_refresh_interval=$3); }
	| STUN_REFRESH_INTERVAL EQUAL error{ yyerror("number expected"); }
	| STUN_ALLOW_STUN EQUAL NUMBER { IF_STUN(stun_allow_stun=$3); }
	| STUN_ALLOW_STUN EQUAL error{ yyerror("number expected"); }
	| STUN_ALLOW_FP EQUAL NUMBER { IF_STUN(stun_allow_fp=$3) ; }
	| STUN_ALLOW_FP EQUAL error{ yyerror("number expected"); }
    | SERVER_ID EQUAL NUMBER { server_id=$3; }
	| UDP_MTU EQUAL NUMBER { default_core_cfg.udp_mtu=$3; }
	| UDP_MTU EQUAL error { yyerror("number expected"); }
	| FORCE_RPORT EQUAL NUMBER 
		{ default_core_cfg.force_rport=$3; fix_global_req_flags(0, 0); }
	| FORCE_RPORT EQUAL error { yyerror("boolean value expected"); }
	| UDP_MTU_TRY_PROTO EQUAL proto
		{ default_core_cfg.udp_mtu_try_proto=$3; fix_global_req_flags(0, 0); }
	| UDP_MTU_TRY_PROTO EQUAL error
		{ yyerror("TCP, TLS, SCTP or UDP expected"); }
	| cfg_var
	| error EQUAL { yyerror("unknown config variable"); }
	;
cfg_var:
	ID DOT ID EQUAL NUMBER {
		if (cfg_declare_int($1, $3, $5, 0, 0, NULL)) {
			yyerror("variable cannot be declared");
		}
	}
	| ID DOT ID EQUAL STRING {
		if (cfg_declare_str($1, $3, $5, NULL)) {
			yyerror("variable cannot be declared");
		}
	}
	| ID DOT ID EQUAL NUMBER CFG_DESCRIPTION STRING {
		if (cfg_declare_int($1, $3, $5, 0, 0, $7)) {
			yyerror("variable cannot be declared");
		}
	}
	| ID DOT ID EQUAL STRING CFG_DESCRIPTION STRING {
		if (cfg_declare_str($1, $3, $5, $7)) {
			yyerror("variable cannot be declared");
		}
	}
	| ID DOT ID EQUAL error { yyerror("number or string expected"); }
	;
module_stm:
	LOADMODULE STRING {
		DBG("loading module %s\n", $2);
			if (load_module($2)!=0) {
				yyerror("failed to load module");
			}
	}
	| LOADMODULE error	{ yyerror("string expected"); }
	| LOADPATH STRING {
		DBG("loading modules under %s\n", $2);
		printf("loading modules under %s\n", $2);
		mods_dir = $2;
	}
	| LOADPATH error	{ yyerror("string expected"); }
	| MODPARAM LPAREN STRING COMMA STRING COMMA STRING RPAREN {
		if (set_mod_param_regex($3, $5, PARAM_STRING, $7) != 0) {
			 yyerror("Can't set module parameter");
		}
	}
        | MODPARAM LPAREN STRING COMMA STRING COMMA NUMBER RPAREN {
		if (set_mod_param_regex($3, $5, PARAM_INT, (void*)$7) != 0) {
			 yyerror("Can't set module parameter");
		}
	}
	| MODPARAM error { yyerror("Invalid arguments"); }
	;
ip:
	ipv4  { $$=$1; }
	| ipv6  { $$=$1; }
	;
ipv4:
	NUMBER DOT NUMBER DOT NUMBER DOT NUMBER {
		$$=pkg_malloc(sizeof(struct ip_addr));
		if ($$==0) {
			LOG(L_CRIT, "ERROR: cfg. parser: out of memory.\n");
		} else {
			memset($$, 0, sizeof(struct ip_addr));
			$$->af=AF_INET;
			$$->len=4;
			if (($1>255) || ($1<0) ||
				($3>255) || ($3<0) ||
				($5>255) || ($5<0) ||
				($7>255) || ($7<0)) {
				yyerror("invalid ipv4 address");
				$$->u.addr32[0]=0;
				/* $$=0; */
			} else {
				$$->u.addr[0]=$1;
				$$->u.addr[1]=$3;
				$$->u.addr[2]=$5;
				$$->u.addr[3]=$7;
				/*
				$$=htonl( ($1<<24)|
				($3<<16)| ($5<<8)|$7 );
				*/
			}
		}
	}
	;
ipv6addr:
	IPV6ADDR {
		$$=pkg_malloc(sizeof(struct ip_addr));
		if ($$==0) {
			LOG(L_CRIT, "ERROR: cfg. parser: out of memory.\n");
		} else {
		#ifdef USE_IPV6
			memset($$, 0, sizeof(struct ip_addr));
			$$->af=AF_INET6;
			$$->len=16;
			if (inet_pton(AF_INET6, $1, $$->u.addr)<=0) {
				yyerror("bad ipv6 address");
			}
		#else
			yyerror("ipv6 address & no ipv6 support compiled in");
			YYABORT;
		#endif
		}
	}
	;
ipv6:
	ipv6addr { $$=$1; }
	| LBRACK ipv6addr RBRACK {$$=$2; }
;


route_name:		NUMBER	{
					tmp=int2str($1, &i_tmp);
					if (($$=pkg_malloc(i_tmp+1))==0) {
						yyerror("out of  memory");
						YYABORT;
					} else {
						memcpy($$, tmp, i_tmp);
						$$[i_tmp]=0;
					}
						}
			|	ID		{ $$=$1; }
			|	STRING	{ $$=$1; }
;

route_stm:
	ROUTE LBRACE actions RBRACE { push($3, &main_rt.rlist[DEFAULT_RT]); }
	| ROUTE LBRACK route_name RBRACK LBRACE actions RBRACE {
		i_tmp=route_get(&main_rt, $3);
		if (i_tmp==-1){
			yyerror("internal error");
			YYABORT;
		}
		if (main_rt.rlist[i_tmp]){
			yyerror("duplicate route");
			YYABORT;
		}
		push($6, &main_rt.rlist[i_tmp]);
	}
	| ROUTE error { yyerror("invalid  route  statement"); }
	;
failure_route_stm:
	ROUTE_FAILURE LBRACE actions RBRACE {
									push($3, &failure_rt.rlist[DEFAULT_RT]);
										}
	| ROUTE_FAILURE LBRACK route_name RBRACK LBRACE actions RBRACE {
		i_tmp=route_get(&failure_rt, $3);
		if (i_tmp==-1){
			yyerror("internal error");
			YYABORT;
		}
		if (failure_rt.rlist[i_tmp]){
			yyerror("duplicate route");
			YYABORT;
		}
		push($6, &failure_rt.rlist[i_tmp]);
	}
	| ROUTE_FAILURE error { yyerror("invalid failure_route statement"); }
	;
onreply_route_stm:
	ROUTE_ONREPLY LBRACE actions RBRACE {
									push($3, &onreply_rt.rlist[DEFAULT_RT]);
										}
	| ROUTE_ONREPLY LBRACK route_name RBRACK LBRACE actions RBRACE {
		i_tmp=route_get(&onreply_rt, $3);
		if (i_tmp==-1){
			yyerror("internal error");
			YYABORT;
		}
		if (onreply_rt.rlist[i_tmp]){
			yyerror("duplicate route");
			YYABORT;
		}
		push($6, &onreply_rt.rlist[i_tmp]);
	}
	| ROUTE_ONREPLY error { yyerror("invalid onreply_route statement"); }
	;
branch_route_stm:
	ROUTE_BRANCH LBRACE actions RBRACE {
									push($3, &branch_rt.rlist[DEFAULT_RT]);
										}
	| ROUTE_BRANCH LBRACK route_name RBRACK LBRACE actions RBRACE {
		i_tmp=route_get(&branch_rt, $3);
		if (i_tmp==-1){
			yyerror("internal error");
			YYABORT;
		}
		if (branch_rt.rlist[i_tmp]){
			yyerror("duplicate route");
			YYABORT;
		}
		push($6, &branch_rt.rlist[i_tmp]);
	}
	| ROUTE_BRANCH error { yyerror("invalid branch_route statement"); }
	;
send_route_stm: ROUTE_SEND LBRACE actions RBRACE {
									push($3, &onsend_rt.rlist[DEFAULT_RT]);
												}
	| ROUTE_SEND LBRACK route_name RBRACK LBRACE actions RBRACE {
		i_tmp=route_get(&onsend_rt, $3);
		if (i_tmp==-1){
			yyerror("internal error");
			YYABORT;
		}
		if (onsend_rt.rlist[i_tmp]){
			yyerror("duplicate route");
			YYABORT;
		}
		push($6, &onsend_rt.rlist[i_tmp]);
	}
	| ROUTE_SEND error { yyerror("invalid onsend_route statement"); }
	;
/*
rules:
	rules rule { push($2, &$1); $$=$1; }
	| rule {$$=$1; }
	| rules error { $$=0; yyerror("invalid rule"); }
	;
rule:
	condition actions CR {
		$$=0;
		if (add_rule($1, $2, &$$)<0) {
			yyerror("error calling add_rule");
			YYABORT;
		}
	}
	| CR	{ $$=0;}
	| condition error { $$=0; yyerror("bad actions in rule"); }
	;
condition:
	exp {$$=$1;}
*/
exp:	exp LOG_AND exp		{ $$=mk_exp(LOGAND_OP, $1, $3); }
	| exp LOG_OR exp	{ $$=mk_exp(LOGOR_OP, $1, $3);  }
	| NOT exp 		{ $$=mk_exp(NOT_OP, $2, 0);  }
	| LPAREN exp RPAREN	{ $$=$2; }
	| exp_elem		{ $$=$1; }
	;
equalop:
	EQUAL_T {$$=EQUAL_OP; }
	| DIFF	{$$=DIFF_OP; }
	;
intop:	equalop	{$$=$1; }
	| GT	{$$=GT_OP; }
	| LT	{$$=LT_OP; }
	| GTE	{$$=GTE_OP; }
	| LTE	{$$=LTE_OP; }
	;
binop :
	BIN_OR { $$= BINOR_OP; }
	| BIN_AND { $$ = BINAND_OP; }
	;
strop:
	equalop	{$$=$1; }
	| MATCH	{$$=MATCH_OP; }
	;
uri_type:
	URI		{$$=URI_O;}
	| FROM_URI	{$$=FROM_URI_O;}
	| TO_URI	{$$=TO_URI_O;}
	;

exp_elem:
	METHOD strop STRING	{$$= mk_elem($2, METHOD_O, 0, STRING_ST, $3);}
	| METHOD strop attr_id_val  {$$ = mk_elem($2, METHOD_O, 0, AVP_ST, $3); }
	| METHOD strop select_id {$$ = mk_elem($2, METHOD_O, 0, SELECT_ST, $3); }
	| METHOD strop  ID	{$$ = mk_elem($2, METHOD_O, 0, STRING_ST,$3); }
	| METHOD strop error { $$=0; yyerror("string expected"); }
	| METHOD error	{ $$=0; yyerror("invalid operator,== , !=, or =~ expected"); }
	| uri_type strop STRING	{$$ = mk_elem($2, $1, 0, STRING_ST, $3); }
	| uri_type strop host 	{$$ = mk_elem($2, $1, 0, STRING_ST, $3); }
	| uri_type strop attr_id_val {$$ = mk_elem($2, $1, 0, AVP_ST, $3); }
	| uri_type strop select_id {$$ = mk_elem($2, $1, 0, SELECT_ST, $3); }
	| uri_type equalop MYSELF {$$=mk_elem($2, $1, 0, MYSELF_ST, 0); }
	| uri_type strop error { $$=0; yyerror("string or MYSELF expected"); }
	| uri_type error	{ $$=0; yyerror("invalid operator, == , != or =~ expected"); }

	| SRCPORT intop NUMBER { $$=mk_elem($2, SRCPORT_O, 0, NUMBER_ST, (void*)$3 ); }
	| SRCPORT intop attr_id_val { $$=mk_elem($2, SRCPORT_O, 0, AVP_ST, (void*)$3 ); }
	| SRCPORT intop error { $$=0; yyerror("number expected"); }
	| SRCPORT error { $$=0; yyerror("==, !=, <,>, >= or <=  expected"); }

	| DSTPORT intop NUMBER	{ $$=mk_elem($2, DSTPORT_O, 0, NUMBER_ST, (void*)$3 ); }
	| DSTPORT intop attr_id_val	{ $$=mk_elem($2, DSTPORT_O, 0, AVP_ST, (void*)$3 ); }
	| DSTPORT intop error { $$=0; yyerror("number expected"); }
	| DSTPORT error { $$=0; yyerror("==, !=, <,>, >= or <=  expected"); }

	| SNDPORT intop NUMBER {
		onsend_check("snd_port");
		$$=mk_elem($2, SNDPORT_O, 0, NUMBER_ST, (void*)$3 );
	}
	| SNDPORT intop attr_id_val {
		onsend_check("snd_port");
		$$=mk_elem($2, SNDPORT_O, 0, AVP_ST, (void*)$3 );
	}
	| SNDPORT intop error { $$=0; yyerror("number expected"); }
	| SNDPORT error { $$=0; yyerror("==, !=, <,>, >= or <=  expected"); }

	| TOPORT intop NUMBER {
		onsend_check("to_port");
		$$=mk_elem($2, TOPORT_O, 0, NUMBER_ST, (void*)$3 );
	}
	| TOPORT intop attr_id_val {
		onsend_check("to_port");
		$$=mk_elem($2, TOPORT_O, 0, AVP_ST, (void*)$3 );
	}
	| TOPORT intop error { $$=0; yyerror("number expected"); }
	| TOPORT error { $$=0; yyerror("==, !=, <,>, >= or <=  expected"); }

	| PROTO intop proto	{ $$=mk_elem($2, PROTO_O, 0, NUMBER_ST, (void*)$3 ); }
	| PROTO intop attr_id_val	{ $$=mk_elem($2, PROTO_O, 0, AVP_ST, (void*)$3 ); }
	| PROTO intop error { $$=0; yyerror("protocol expected (udp, tcp or tls)"); }

	| PROTO error { $$=0; yyerror("equal/!= operator expected"); }

	| SNDPROTO intop proto	{
		onsend_check("snd_proto");
		$$=mk_elem($2, SNDPROTO_O, 0, NUMBER_ST, (void*)$3 );
	}
	| SNDPROTO intop attr_id_val {
		onsend_check("snd_proto");
		$$=mk_elem($2, SNDPROTO_O, 0, AVP_ST, (void*)$3 );
	}
	| SNDPROTO intop error { $$=0; yyerror("protocol expected (udp, tcp or tls)"); }
	| SNDPROTO error { $$=0; yyerror("equal/!= operator expected"); }

	| AF intop NUMBER	{ $$=mk_elem($2, AF_O, 0, NUMBER_ST,(void *) $3 ); }
	| AF intop attr_id_val	{ $$=mk_elem($2, AF_O, 0, AVP_ST,(void *) $3 ); }
	| AF intop error { $$=0; yyerror("number expected"); }
	| AF error { $$=0; yyerror("equal/!= operator expected"); }

	| SNDAF intop NUMBER {
		onsend_check("snd_af");
		$$=mk_elem($2, SNDAF_O, 0, NUMBER_ST, (void *) $3 ); }
	| SNDAF intop attr_id_val {
		onsend_check("snd_af");
		$$=mk_elem($2, SNDAF_O, 0, AVP_ST, (void *) $3 );
	}
	| SNDAF intop error { $$=0; yyerror("number expected"); }
	| SNDAF error { $$=0; yyerror("equal/!= operator expected"); }

	| MSGLEN intop NUMBER		{ $$=mk_elem($2, MSGLEN_O, 0, NUMBER_ST, (void *) $3 ); }
	| MSGLEN intop attr_id_val	{ $$=mk_elem($2, MSGLEN_O, 0, AVP_ST, (void *) $3 ); }
	| MSGLEN intop MAX_LEN		{ $$=mk_elem($2, MSGLEN_O, 0, NUMBER_ST, (void *) BUF_SIZE); }
	| MSGLEN intop error { $$=0; yyerror("number expected"); }
	| MSGLEN error { $$=0; yyerror("equal/!= operator expected"); }

	| RETCODE intop NUMBER	{ $$=mk_elem($2, RETCODE_O, 0, NUMBER_ST, (void *) $3 ); }
	| RETCODE intop attr_id_val	{ $$=mk_elem($2, RETCODE_O, 0, AVP_ST, (void *) $3 ); }
	| RETCODE intop error { $$=0; yyerror("number expected"); }
	| RETCODE error { $$=0; yyerror("equal/!= operator expected"); }

	| SRCIP equalop ipnet	{ $$=mk_elem($2, SRCIP_O, 0, NET_ST, $3); }
	| SRCIP strop STRING {
		s_tmp.s=$3;
		s_tmp.len=strlen($3);
		ip_tmp=str2ip(&s_tmp);
	#ifdef USE_IPV6
		if (ip_tmp==0)
			ip_tmp=str2ip6(&s_tmp);
	#endif
		if (ip_tmp) {
			$$=mk_elem($2, SRCIP_O, 0, NET_ST, mk_net_bitlen(ip_tmp, ip_tmp->len*8) );
		} else {
			$$=mk_elem($2, SRCIP_O, 0, STRING_ST, $3);
		}
	}
	| SRCIP strop host	{ $$=mk_elem($2, SRCIP_O, 0, STRING_ST, $3); }
	| SRCIP equalop MYSELF  { $$=mk_elem($2, SRCIP_O, 0, MYSELF_ST, 0);
							}
	| SRCIP strop error { $$=0; yyerror( "ip address or hostname expected" ); }
	| SRCIP error  { $$=0; yyerror("invalid operator, ==, != or =~ expected");}
	| DSTIP equalop ipnet	{ $$=mk_elem(	$2, DSTIP_O, 0, NET_ST, (void*)$3); }
	| DSTIP strop STRING	{
		s_tmp.s=$3;
		s_tmp.len=strlen($3);
		ip_tmp=str2ip(&s_tmp);
	#ifdef USE_IPV6
		if (ip_tmp==0)
			ip_tmp=str2ip6(&s_tmp);
	#endif /* USE_IPV6 */
		if (ip_tmp) {
			$$=mk_elem($2, DSTIP_O, 0, NET_ST, mk_net_bitlen(ip_tmp, ip_tmp->len*8) );
		} else {
			$$=mk_elem($2, DSTIP_O, 0, STRING_ST, $3);
		}
	}
	| DSTIP strop host	{ $$=mk_elem(	$2, DSTIP_O, 0, STRING_ST, $3); }
	| DSTIP equalop MYSELF  { $$=mk_elem(	$2, DSTIP_O, 0, MYSELF_ST, 0); }
	| DSTIP strop error { $$=0; yyerror( "ip address or hostname expected" ); }
	| DSTIP error { $$=0; yyerror("invalid operator, ==, != or =~ expected"); }
	| SNDIP equalop ipnet {
		onsend_check("snd_ip");
		$$=mk_elem($2, SNDIP_O, 0, NET_ST, $3);
	}
	| SNDIP strop STRING	{
		onsend_check("snd_ip");
		s_tmp.s=$3;
		s_tmp.len=strlen($3);
		ip_tmp=str2ip(&s_tmp);
	#ifdef USE_IPV6
		if (ip_tmp==0)
			ip_tmp=str2ip6(&s_tmp);
	#endif /* USE_IPV6 */
		if (ip_tmp) {
			$$=mk_elem($2, SNDIP_O, 0, NET_ST, mk_net_bitlen(ip_tmp, ip_tmp->len*8) );
		} else {
			$$=mk_elem($2, SNDIP_O, 0, STRING_ST, $3);
		}
	}
	| SNDIP strop host	{
		onsend_check("snd_ip");
		$$=mk_elem($2, SNDIP_O, 0, STRING_ST, $3);
	}
	| SNDIP equalop attr_id_val	{
		onsend_check("snd_ip");
	    $$=mk_elem($2, SNDIP_O, 0, AVP_ST, (void*)$3 ); 
	}
	| SNDIP equalop MYSELF  {
		onsend_check("snd_ip");
		$$=mk_elem($2, SNDIP_O, 0, MYSELF_ST, 0);
	}
	| SNDIP strop error { $$=0; yyerror( "ip address or hostname expected" ); }
	| SNDIP error  { $$=0; yyerror("invalid operator, ==, != or =~ expected"); }
	| TOIP equalop ipnet	{
		onsend_check("to_ip");
		$$=mk_elem($2, TOIP_O, 0, NET_ST, $3);
	}
	| TOIP strop STRING	{
		onsend_check("to_ip");
		s_tmp.s=$3;
		s_tmp.len=strlen($3);
		ip_tmp=str2ip(&s_tmp);
	#ifdef USE_IPV6
		if (ip_tmp==0)
			ip_tmp=str2ip6(&s_tmp);
	#endif /* USE_IPV6 */
		if (ip_tmp) {
			$$=mk_elem($2, TOIP_O, 0, NET_ST, mk_net_bitlen(ip_tmp, ip_tmp->len*8) );
		} else {
			$$=mk_elem($2, TOIP_O, 0, STRING_ST, $3);
		}
	}
	| TOIP strop host	{
		onsend_check("to_ip");
		$$=mk_elem($2, TOIP_O, 0, STRING_ST, $3);
	}
	| TOIP equalop attr_id_val	{
		onsend_check("to_ip");
	    $$=mk_elem($2, TOIP_O, 0, AVP_ST, (void*)$3 ); 
	}
	| TOIP equalop MYSELF  {
		onsend_check("to_ip");
		$$=mk_elem($2, TOIP_O, 0, MYSELF_ST, 0);
	}
	| TOIP strop error { $$=0; yyerror( "ip address or hostname expected" ); }
	| TOIP error  { $$=0; yyerror("invalid operator, ==, != or =~ expected"); }

	| MYSELF equalop uri_type	{ $$=mk_elem($2, $3, 0, MYSELF_ST, 0); }
	| MYSELF equalop SRCIP  { $$=mk_elem($2, SRCIP_O, 0, MYSELF_ST, 0); }
	| MYSELF equalop DSTIP  { $$=mk_elem($2, DSTIP_O, 0, MYSELF_ST, 0); }
	| MYSELF equalop SNDIP  {
		onsend_check("snd_ip");
		$$=mk_elem($2, SNDIP_O, 0, MYSELF_ST, 0);
	}
	| MYSELF equalop TOIP  {
		onsend_check("to_ip");
		$$=mk_elem($2, TOIP_O, 0, MYSELF_ST, 0);
	}
	| MYSELF equalop error { $$=0; yyerror(" URI, SRCIP or DSTIP expected"); }
	| MYSELF error	{ $$=0; yyerror ("invalid operator, == or != expected"); }
	| exp_stm	{ $$=mk_elem( NO_OP, ACTION_O, 0, ACTIONS_ST, $1);  }
	| NUMBER	{ $$=mk_elem( NO_OP, NUMBER_O, 0, NUMBER_ST, (void*)$1 ); }

	| attr_id_any				{$$=mk_elem( NO_OP, AVP_O, (void*)$1, 0, 0); }
	| attr_id_val strop STRING	{$$=mk_elem( $2, AVP_O, (void*)$1, STRING_ST, $3); }
	| attr_id_val strop select_id	{$$=mk_elem( $2, AVP_O, (void*)$1, SELECT_ST, $3); }
	| attr_id_val intop NUMBER	{$$=mk_elem( $2, AVP_O, (void*)$1, NUMBER_ST, (void*)$3); }
	| attr_id_val binop NUMBER	{$$=mk_elem( $2, AVP_O, (void*)$1, NUMBER_ST, (void*)$3); }
	| attr_id_val strop attr_id_val {$$=mk_elem( $2, AVP_O, (void*)$1, AVP_ST, (void*)$3); }
	| attr_id_val intop attr_id_val {$$=mk_elem( $2, AVP_O, (void*)$1, AVP_ST, (void*)$3); }

	| select_id                 { $$=mk_elem( NO_OP, SELECT_O, $1, 0, 0); }
	| select_id strop STRING    { $$=mk_elem( $2, SELECT_O, $1, STRING_ST, $3); }
	| select_id strop attr_id_val   { $$=mk_elem( $2, SELECT_O, $1, AVP_ST, (void*)$3); }
	| select_id strop select_id { $$=mk_elem( $2, SELECT_O, $1, SELECT_ST, $3); }
;
ipnet:
	ip SLASH ip	{ $$=mk_net($1, $3); }
	| ip SLASH NUMBER {
		if (($3<0) || ($3>$1->len*8)) {
			yyerror("invalid bit number in netmask");
			$$=0;
		} else {
			$$=mk_net_bitlen($1, $3);
		/*
			$$=mk_net($1, htonl( ($3)?~( (1<<(32-$3))-1 ):0 ) );
		*/
		}
	}
	| ip	{ $$=mk_net_bitlen($1, $1->len*8); }
	| ip SLASH error { $$=0; yyerror("netmask (eg:255.0.0.0 or 8) expected"); }
	;
host_sep:
	DOT {$$=".";}
	| MINUS {$$="-"; }
	;

host:
	ID { $$=$1; }
	| host host_sep ID {
		if ($1){
			$$=(char*)pkg_malloc(strlen($1)+1+strlen($3)+1);
			if ($$==0) {
				LOG(L_CRIT, "ERROR: cfg. parser: memory allocation"
							" failure while parsing host\n");
			} else {
				memcpy($$, $1, strlen($1));
				$$[strlen($1)]=*$2;
				memcpy($$+strlen($1)+1, $3, strlen($3));
				$$[strlen($1)+1+strlen($3)]=0;
			}
			pkg_free($1);
		}
		if ($3) pkg_free($3);
	}
	| host DOT error { $$=0; pkg_free($1); yyerror("invalid hostname"); }
	;

host_if_id: ID
		| NUM_ID
		| NUMBER {
			/* get string version */
			$$=pkg_malloc(strlen(yy_number_str)+1);
			if ($$)
				strcpy($$, yy_number_str);
		}
		;

host_or_if:
	host_if_id { $$=$1; }
	| host_or_if host_sep host_if_id {
		if ($1){
			$$=(char*)pkg_malloc(strlen($1)+1+strlen($3)+1);
			if ($$==0) {
				LOG(L_CRIT, "ERROR: cfg. parser: memory allocation"
							" failure while parsing host/interface name\n");
			} else {
				memcpy($$, $1, strlen($1));
				$$[strlen($1)]=*$2;
				memcpy($$+strlen($1)+1, $3, strlen($3));
				$$[strlen($1)+1+strlen($3)]=0;
			}
			pkg_free($1);
		}
		if ($3) pkg_free($3);
	}
	| host_or_if host_sep error { $$=0; pkg_free($1);
								yyerror("invalid host or interface name"); }
	;


/* filtered cmd */
fcmd:
	cmd {
		/* check if allowed */
		if ($1 && rt==ONSEND_ROUTE) {
			switch($1->type) {
				case DROP_T:
				case SEND_T:
				case SEND_TCP_T:
				case LOG_T:
				case SETFLAG_T:
				case RESETFLAG_T:
				case ISFLAGSET_T:
				case IF_T:
				case MODULE_T:
					$$=$1;
					break;
				default:
					$$=0;
					yyerror("command not allowed in onsend_route\n");
			}
		} else {
			$$=$1;
		}
	}
	;
exp_stm:
	fcmd	{ $$=$1; }
	| if_cmd	{ $$=$1; }
	| assign_action { $$ = $1; }
	| LBRACE actions RBRACE	{ $$=$2; }
	;
stm:
	action	{ $$=$1; }
	| LBRACE actions RBRACE	{ $$=$2; }
	;
actions:
	actions action	{$$=append_action($1, $2); }
	| action	{$$=$1;}
	| actions error { $$=0; yyerror("bad command"); }
	;
action:
	fcmd SEMICOLON {$$=$1;}
	| if_cmd {$$=$1;}
	| assign_action SEMICOLON {$$=$1;}
	| SEMICOLON /* null action */ {$$=0;}
	| fcmd error { $$=0; yyerror("bad command: missing ';'?"); }
	;
if_cmd:
	IF exp stm		{ $$=mk_action( IF_T, 3, EXPR_ST, $2, ACTIONS_ST, $3, NOSUBTYPE, 0); }
	| IF exp stm ELSE stm	{ $$=mk_action( IF_T, 3, EXPR_ST, $2, ACTIONS_ST, $3, ACTIONS_ST, $5); }
	;
/* class_id:
	LBRACK ATTR_USER RBRACK { $$ = AVP_CLASS_USER; }
	| LBRACK ATTR_DOMAIN RBRACK { $$ = AVP_CLASS_DOMAIN; }
	| LBRACK ATTR_GLOBAL RBRACK { $$ = AVP_CLASS_GLOBAL; }
	;
*/
select_param:
	ID {
		if (sel.n >= MAX_SELECT_PARAMS-1) {
			yyerror("Select identifier too long\n");
		}
		sel.params[sel.n].type = SEL_PARAM_STR;
		sel.params[sel.n].v.s.s = $1;
		sel.params[sel.n].v.s.len = strlen($1);
		sel.n++;
	}
	| ID LBRACK NUMBER RBRACK {
		if (sel.n >= MAX_SELECT_PARAMS-2) {
			yyerror("Select identifier too long\n");
		}
		sel.params[sel.n].type = SEL_PARAM_STR;
		sel.params[sel.n].v.s.s = $1;
		sel.params[sel.n].v.s.len = strlen($1);
		sel.n++;
		sel.params[sel.n].type = SEL_PARAM_INT;
		sel.params[sel.n].v.i = $3;
		sel.n++;
	}
	| ID LBRACK STRING RBRACK {
		if (sel.n >= MAX_SELECT_PARAMS-2) {
			yyerror("Select identifier too long\n");
		}
		sel.params[sel.n].type = SEL_PARAM_STR;
		sel.params[sel.n].v.s.s = $1;
		sel.params[sel.n].v.s.len = strlen($1);
		sel.n++;
		sel.params[sel.n].type = SEL_PARAM_STR;
		sel.params[sel.n].v.s.s = $3;
		sel.params[sel.n].v.s.len = strlen($3);
		sel.n++;
	}
	;
select_params:
	select_params DOT select_param
	| select_param
	;
select_id:
	SELECT_MARK { sel.n = 0; sel.f[0] = 0; } select_params {
		sel_ptr = (select_t*)pkg_malloc(sizeof(select_t));
		if (!sel_ptr) {
			yyerror("No memory left to allocate select structure\n");
		}
		memcpy(sel_ptr, &sel, sizeof(select_t));
		$$ = sel_ptr;
	}
	;
attr_class_spec:
	ATTR_FROM { s_attr->type |= AVP_TRACK_FROM; }
	| ATTR_TO { s_attr->type |= AVP_TRACK_TO; }
        | ATTR_FROMURI { s_attr->type |= AVP_TRACK_FROM | AVP_CLASS_URI; }
        | ATTR_TOURI { s_attr->type |= AVP_TRACK_TO | AVP_CLASS_URI; }
	| ATTR_FROMUSER { s_attr->type |= AVP_TRACK_FROM | AVP_CLASS_USER; }
	| ATTR_TOUSER { s_attr->type |= AVP_TRACK_TO | AVP_CLASS_USER; }
	| ATTR_FROMDOMAIN { s_attr->type |= AVP_TRACK_FROM | AVP_CLASS_DOMAIN; }
	| ATTR_TODOMAIN { s_attr->type |= AVP_TRACK_TO | AVP_CLASS_DOMAIN; }
	| ATTR_GLOBAL { s_attr->type |= AVP_TRACK_ALL | AVP_CLASS_GLOBAL; }
	;
attr_name_spec:
	ID { s_attr->type |= AVP_NAME_STR; s_attr->name.s.s = $1; s_attr->name.s.len = strlen ($1); }
	;
attr_spec:
	attr_name_spec
	| attr_class_spec DOT attr_name_spec
	;
attr_mark:
	ATTR_MARK {
		s_attr = (struct avp_spec*)pkg_malloc(sizeof(struct avp_spec));
		if (!s_attr) { yyerror("No memory left"); }
		s_attr->type = 0;
	}
	;
attr_id:
	attr_mark attr_spec { $$ = s_attr; }
	;
attr_id_num_idx:
	attr_mark attr_spec LBRACK NUMBER RBRACK {
		s_attr->type|= (AVP_NAME_STR | ($4<0?AVP_INDEX_BACKWARD:AVP_INDEX_FORWARD));
		s_attr->index = ($4<0?-$4:$4);
		$$ = s_attr;
	}
	;
attr_id_no_idx:
	attr_mark attr_spec LBRACK RBRACK {
		s_attr->type|= AVP_INDEX_ALL;
		$$ = s_attr;
	}
	;
attr_id_ass:
	attr_id
	| attr_id_no_idx
	;
attr_id_val:
	attr_id
	| attr_id_num_idx
	;
attr_id_any:
	attr_id
	| attr_id_no_idx
	| attr_id_num_idx
;
attr_id_any_str:
	attr_id
	| STRING {
		avp_spec_t *avp_spec;
		str s;
		int type, idx;
		avp_spec = pkg_malloc(sizeof(*avp_spec));
		if (!avp_spec) {
			yyerror("Not enough memory");
			YYABORT;
		}
		s.s = $1;
		if (s.s[0] == '$')
			s.s++;
		s.len = strlen(s.s);
		if (parse_avp_name(&s, &type, &avp_spec->name, &idx)) {
			yyerror("error when parsing AVP");
		        pkg_free(avp_spec);
			YYABORT;
		}
		avp_spec->type = type;
		avp_spec->index = idx;
		$$ = avp_spec;
	}
	;
/*
assign_op:
	ADDEQ { $$ = ADD_T; }
	| EQUAL { $$ = ASSIGN_T; }
	;
*/
assign_op:
	EQUAL { $$ = ASSIGN_T; }
	;
assign_action:
	attr_id_ass assign_op STRING  { $$=mk_action($2, 2, AVP_ST, $1, STRING_ST, $3); }
	| attr_id_ass assign_op NUMBER  { $$=mk_action($2, 2, AVP_ST, $1, NUMBER_ST, (void*)$3); }
	| attr_id_ass assign_op fcmd    { $$=mk_action($2, 2, AVP_ST, $1, ACTION_ST, $3); }
	| attr_id_ass assign_op attr_id_any { $$=mk_action($2, 2, AVP_ST, $1, AVP_ST, $3); }
	| attr_id_ass assign_op select_id { $$=mk_action($2, 2, AVP_ST, (void*)$1, SELECT_ST, (void*)$3); }
	| attr_id_ass assign_op LPAREN exp RPAREN { $$ = mk_action($2, 2, AVP_ST, $1, EXPR_ST, $4); }
	;
avpflag_oper:
	SETAVPFLAG { $$ = 1; }
	| RESETAVPFLAG { $$ = 0; }
	| ISAVPFLAGSET { $$ = -1; }
	;
cmd:
	FORWARD LPAREN host RPAREN	{ $$=mk_action(	FORWARD_T, 2, STRING_ST, $3, NUMBER_ST, 0); }
	| FORWARD LPAREN STRING RPAREN	{ $$=mk_action(	FORWARD_T, 2, STRING_ST, $3, NUMBER_ST, 0); }
	| FORWARD LPAREN ip RPAREN	{ $$=mk_action(	FORWARD_T, 2, IP_ST, (void*)$3, NUMBER_ST, 0); }
	| FORWARD LPAREN host COMMA NUMBER RPAREN { $$=mk_action(FORWARD_T, 2, STRING_ST, $3, NUMBER_ST, (void*)$5); }
	| FORWARD LPAREN STRING COMMA NUMBER RPAREN {$$=mk_action(FORWARD_T, 2, STRING_ST, $3, NUMBER_ST, (void*)$5); }
	| FORWARD LPAREN ip COMMA NUMBER RPAREN { $$=mk_action(FORWARD_T, 2, IP_ST, (void*)$3, NUMBER_ST, (void*)$5); }
	| FORWARD LPAREN URIHOST COMMA URIPORT RPAREN { $$=mk_action(FORWARD_T, 2, URIHOST_ST, 0, URIPORT_ST, 0); }
	| FORWARD LPAREN URIHOST COMMA NUMBER RPAREN {$$=mk_action(FORWARD_T, 2, URIHOST_ST, 0, NUMBER_ST, (void*)$5); }
	| FORWARD LPAREN URIHOST RPAREN { $$=mk_action(FORWARD_T, 2, URIHOST_ST, 0, NUMBER_ST, 0); }
	| FORWARD error { $$=0; yyerror("missing '(' or ')' ?"); }
	| FORWARD LPAREN error RPAREN { $$=0; yyerror("bad forward argument"); }
	| FORWARD_UDP LPAREN host RPAREN	{ $$=mk_action(FORWARD_UDP_T, 2, STRING_ST, $3, NUMBER_ST, 0); }
	| FORWARD_UDP LPAREN STRING RPAREN	{ $$=mk_action(FORWARD_UDP_T, 2, STRING_ST, $3, NUMBER_ST, 0); }
	| FORWARD_UDP LPAREN ip RPAREN	{ $$=mk_action(FORWARD_UDP_T, 2, IP_ST, (void*)$3, NUMBER_ST, 0); }
	| FORWARD_UDP LPAREN host COMMA NUMBER RPAREN { $$=mk_action(FORWARD_UDP_T, 2, STRING_ST, $3, NUMBER_ST, (void*)$5); }
	| FORWARD_UDP LPAREN STRING COMMA NUMBER RPAREN {$$=mk_action(FORWARD_UDP_T, 2, STRING_ST, $3, NUMBER_ST, (void*)$5); }
	| FORWARD_UDP LPAREN ip COMMA NUMBER RPAREN { $$=mk_action(FORWARD_UDP_T, 2, IP_ST, (void*)$3, NUMBER_ST, (void*)$5); }
	| FORWARD_UDP LPAREN URIHOST COMMA URIPORT RPAREN {$$=mk_action(FORWARD_UDP_T, 2, URIHOST_ST, 0, URIPORT_ST, 0); }
	| FORWARD_UDP LPAREN URIHOST COMMA NUMBER RPAREN { $$=mk_action(FORWARD_UDP_T, 2, URIHOST_ST, 0, NUMBER_ST, (void*)$5); }
	| FORWARD_UDP LPAREN URIHOST RPAREN { $$=mk_action(FORWARD_UDP_T, 2, URIHOST_ST, 0, NUMBER_ST, 0); }
	| FORWARD_UDP error { $$=0; yyerror("missing '(' or ')' ?"); }
	| FORWARD_UDP LPAREN error RPAREN { $$=0; yyerror("bad forward_udp argument"); }
	| FORWARD_TCP LPAREN host RPAREN	{ $$=mk_action(FORWARD_TCP_T, 2, STRING_ST, $3, NUMBER_ST, 0); }
	| FORWARD_TCP LPAREN STRING RPAREN	{ $$=mk_action(FORWARD_TCP_T, 2, STRING_ST, $3, NUMBER_ST, 0); }
	| FORWARD_TCP LPAREN ip RPAREN	{ $$=mk_action(FORWARD_TCP_T, 2, IP_ST, (void*)$3, NUMBER_ST, 0); }
	| FORWARD_TCP LPAREN host COMMA NUMBER RPAREN { $$=mk_action(FORWARD_TCP_T, 2, STRING_ST, $3, NUMBER_ST, (void*)$5); }
	| FORWARD_TCP LPAREN STRING COMMA NUMBER RPAREN {$$=mk_action(FORWARD_TCP_T, 2, STRING_ST, $3, NUMBER_ST, (void*)$5); }
	| FORWARD_TCP LPAREN ip COMMA NUMBER RPAREN { $$=mk_action(FORWARD_TCP_T, 2, IP_ST, (void*)$3, NUMBER_ST, (void*)$5); }
	| FORWARD_TCP LPAREN URIHOST COMMA URIPORT RPAREN {$$=mk_action(FORWARD_TCP_T, 2, URIHOST_ST, 0, URIPORT_ST, 0); }
	| FORWARD_TCP LPAREN URIHOST COMMA NUMBER RPAREN { $$=mk_action(FORWARD_TCP_T, 2, URIHOST_ST, 0, NUMBER_ST, (void*)$5); }
	| FORWARD_TCP LPAREN URIHOST RPAREN { $$=mk_action(FORWARD_TCP_T, 2, URIHOST_ST, 0, NUMBER_ST, 0); }
	| FORWARD_TCP error { $$=0; yyerror("missing '(' or ')' ?"); }
	| FORWARD_TCP LPAREN error RPAREN { $$=0; yyerror("bad forward_tcp argument"); }
	| FORWARD_TLS LPAREN host RPAREN {
		#ifdef USE_TLS
			$$=mk_action(FORWARD_TLS_T, 2, STRING_ST, $3, NUMBER_ST, 0);
		#else
			$$=0;
			yyerror("tls support not compiled in");
		#endif
	}
	| FORWARD_TLS LPAREN STRING RPAREN {
		#ifdef USE_TLS
			$$=mk_action(FORWARD_TLS_T, 2, STRING_ST, $3, NUMBER_ST, 0);
		#else
			$$=0;
			yyerror("tls support not compiled in");
		#endif
	}
	| FORWARD_TLS LPAREN ip RPAREN	{
		#ifdef USE_TLS
			$$=mk_action(FORWARD_TLS_T, 2, IP_ST, (void*)$3, NUMBER_ST, 0);
		#else
			$$=0;
			yyerror("tls support not compiled in");
		#endif
	}
	| FORWARD_TLS LPAREN host COMMA NUMBER RPAREN {
		#ifdef USE_TLS
			$$=mk_action(FORWARD_TLS_T, 2, STRING_ST, $3, NUMBER_ST, (void*)$5);
		#else
			$$=0;
			yyerror("tls support not compiled in");
		#endif
	}
	| FORWARD_TLS LPAREN STRING COMMA NUMBER RPAREN {
		#ifdef USE_TLS
			$$=mk_action(FORWARD_TLS_T, 2, STRING_ST, $3, NUMBER_ST, (void*)$5);
		#else
			$$=0;
			yyerror("tls support not compiled in");
		#endif
	}
	| FORWARD_TLS LPAREN ip COMMA NUMBER RPAREN {
		#ifdef USE_TLS
			$$=mk_action(FORWARD_TLS_T, 2, IP_ST, (void*)$3, NUMBER_ST, (void*)$5);
		#else
			$$=0;
			yyerror("tls support not compiled in");
		#endif
					}
	| FORWARD_TLS LPAREN URIHOST COMMA URIPORT RPAREN {
		#ifdef USE_TLS
			$$=mk_action(FORWARD_TLS_T, 2, URIHOST_ST, 0, URIPORT_ST, 0);
		#else
			$$=0;
			yyerror("tls support not compiled in");
		#endif
	}
	| FORWARD_TLS LPAREN URIHOST COMMA NUMBER RPAREN {
		#ifdef USE_TLS
			$$=mk_action(FORWARD_TLS_T, 2, URIHOST_ST, 0, NUMBER_ST, (void*)$5);
		#else
			$$=0;
			yyerror("tls support not compiled in");
		#endif
	}
	| FORWARD_TLS LPAREN URIHOST RPAREN {
		#ifdef USE_TLS
			$$=mk_action(FORWARD_TLS_T, 2, URIHOST_ST, 0, NUMBER_ST, 0);
		#else
			$$=0;
			yyerror("tls support not compiled in");
		#endif
	}
	| FORWARD_TLS error { $$=0; yyerror("missing '(' or ')' ?"); }
	| FORWARD_TLS LPAREN error RPAREN { $$=0; 
									yyerror("bad forward_tls argument"); }
	| FORWARD_SCTP LPAREN host RPAREN {
		#ifdef USE_SCTP
			$$=mk_action(FORWARD_SCTP_T, 2, STRING_ST, $3, NUMBER_ST, 0);
		#else
			$$=0;
			yyerror("sctp support not compiled in");
		#endif
	}
	| FORWARD_SCTP LPAREN STRING RPAREN {
		#ifdef USE_SCTP
			$$=mk_action(FORWARD_SCTP_T, 2, STRING_ST, $3, NUMBER_ST, 0);
		#else
			$$=0;
			yyerror("sctp support not compiled in");
		#endif
	}
	| FORWARD_SCTP LPAREN ip RPAREN	{
		#ifdef USE_SCTP
			$$=mk_action(FORWARD_SCTP_T, 2, IP_ST, (void*)$3, NUMBER_ST, 0);
		#else
			$$=0;
			yyerror("sctp support not compiled in");
		#endif
	}
	| FORWARD_SCTP LPAREN host COMMA NUMBER RPAREN {
		#ifdef USE_SCTP
			$$=mk_action(FORWARD_SCTP_T, 2, STRING_ST, $3, NUMBER_ST,
							(void*)$5);
		#else
			$$=0;
			yyerror("sctp support not compiled in");
		#endif
	}
	| FORWARD_SCTP LPAREN STRING COMMA NUMBER RPAREN {
		#ifdef USE_SCTP
			$$=mk_action(FORWARD_SCTP_T, 2, STRING_ST, $3, NUMBER_ST,
							(void*)$5);
		#else
			$$=0;
			yyerror("sctp support not compiled in");
		#endif
	}
	| FORWARD_SCTP LPAREN ip COMMA NUMBER RPAREN {
		#ifdef USE_SCTP
			$$=mk_action(FORWARD_SCTP_T, 2, IP_ST, (void*)$3, NUMBER_ST, 
							(void*)$5);
		#else
			$$=0;
			yyerror("sctp support not compiled in");
		#endif
					}
	| FORWARD_SCTP LPAREN URIHOST COMMA URIPORT RPAREN {
		#ifdef USE_SCTP
			$$=mk_action(FORWARD_SCTP_T, 2, URIHOST_ST, 0, URIPORT_ST, 0);
		#else
			$$=0;
			yyerror("sctp support not compiled in");
		#endif
	}
	| FORWARD_SCTP LPAREN URIHOST COMMA NUMBER RPAREN {
		#ifdef USE_SCTP
			$$=mk_action(FORWARD_SCTP_T, 2, URIHOST_ST, 0, NUMBER_ST,
							(void*)$5);
		#else
			$$=0;
			yyerror("sctp support not compiled in");
		#endif
	}
	| FORWARD_SCTP LPAREN URIHOST RPAREN {
		#ifdef USE_SCTP
			$$=mk_action(FORWARD_SCTP_T, 2, URIHOST_ST, 0, NUMBER_ST, 0);
		#else
			$$=0;
			yyerror("tls support not compiled in");
		#endif
	}
	| FORWARD_SCTP error { $$=0; yyerror("missing '(' or ')' ?"); }
	| FORWARD_SCTP LPAREN error RPAREN { $$=0; 
									yyerror("bad forward_tls argument"); }
	| SEND LPAREN host RPAREN	{ $$=mk_action(SEND_T, 2, STRING_ST, $3, NUMBER_ST, 0); }
	| SEND LPAREN STRING RPAREN { $$=mk_action(SEND_T, 2, STRING_ST, $3, NUMBER_ST, 0); }
	| SEND LPAREN ip RPAREN		{ $$=mk_action(SEND_T, 2, IP_ST, (void*)$3, NUMBER_ST, 0); }
	| SEND LPAREN host COMMA NUMBER RPAREN	{ $$=mk_action(SEND_T, 2, STRING_ST, $3, NUMBER_ST, (void*)$5); }
	| SEND LPAREN STRING COMMA NUMBER RPAREN {$$=mk_action(SEND_T, 2, STRING_ST, $3, NUMBER_ST, (void*)$5); }
	| SEND LPAREN ip COMMA NUMBER RPAREN { $$=mk_action(SEND_T, 2, IP_ST, (void*)$3, NUMBER_ST, (void*)$5); }
	| SEND error { $$=0; yyerror("missing '(' or ')' ?"); }
	| SEND LPAREN error RPAREN { $$=0; yyerror("bad send argument"); }
	| SEND_TCP LPAREN host RPAREN	{ $$=mk_action(SEND_TCP_T, 2, STRING_ST, $3, NUMBER_ST, 0); }
	| SEND_TCP LPAREN STRING RPAREN { $$=mk_action(SEND_TCP_T, 2, STRING_ST, $3, NUMBER_ST, 0); }
	| SEND_TCP LPAREN ip RPAREN	{ $$=mk_action(SEND_TCP_T, 2, IP_ST, (void*)$3, NUMBER_ST, 0); }
	| SEND_TCP LPAREN host COMMA NUMBER RPAREN	{ $$=mk_action(	SEND_TCP_T, 2, STRING_ST, $3, NUMBER_ST, (void*)$5);}
	| SEND_TCP LPAREN STRING COMMA NUMBER RPAREN {$$=mk_action(SEND_TCP_T, 2, STRING_ST, $3, NUMBER_ST, (void*)$5); }
	| SEND_TCP LPAREN ip COMMA NUMBER RPAREN { $$=mk_action(SEND_TCP_T, 2, IP_ST, (void*)$3, NUMBER_ST, (void*)$5); }
	| SEND_TCP error { $$=0; yyerror("missing '(' or ')' ?"); }
	| SEND_TCP LPAREN error RPAREN { $$=0; yyerror("bad send_tcp argument"); }
	| DROP LPAREN RPAREN		{$$=mk_action(DROP_T, 2, NUMBER_ST, 0, NUMBER_ST, (void*)EXIT_R_F); }
	| DROP LPAREN NUMBER RPAREN	{$$=mk_action(DROP_T, 2, NUMBER_ST, (void*)$3, NUMBER_ST, (void*)EXIT_R_F); }
	| DROP NUMBER 			{$$=mk_action(DROP_T, 2, NUMBER_ST, (void*)$2, NUMBER_ST, (void*)EXIT_R_F); }
	| DROP RETCODE 			{$$=mk_action(DROP_T, 2, RETCODE_ST, 0, NUMBER_ST, (void*)EXIT_R_F); }
	| DROP				{$$=mk_action(DROP_T, 2, NUMBER_ST, 0, NUMBER_ST, (void*)EXIT_R_F); }
	| RETURN			{$$=mk_action(DROP_T, 2, NUMBER_ST, (void*)1, NUMBER_ST, (void*)RETURN_R_F); }
	| RETURN NUMBER			{$$=mk_action(DROP_T, 2, NUMBER_ST, (void*)$2, NUMBER_ST, (void*)RETURN_R_F);}
	| RETURN RETCODE		{$$=mk_action(DROP_T, 2, RETCODE_ST, 0, NUMBER_ST, (void*)RETURN_R_F);}
	| BREAK				{$$=mk_action(DROP_T, 2, NUMBER_ST, 0, NUMBER_ST, (void*)RETURN_R_F); }
	| LOG_TOK LPAREN STRING RPAREN	{$$=mk_action(LOG_T, 2, NUMBER_ST,
										(void*)(L_DBG+1), STRING_ST, $3); }
	| LOG_TOK LPAREN NUMBER COMMA STRING RPAREN	{$$=mk_action(LOG_T, 2, NUMBER_ST, (void*)$3, STRING_ST, $5); }
	| LOG_TOK error 		{ $$=0; yyerror("missing '(' or ')' ?"); }
	| LOG_TOK LPAREN error RPAREN	{ $$=0; yyerror("bad log argument"); }
	| SETFLAG LPAREN NUMBER RPAREN	{
							if (check_flag($3)==-1)
								yyerror("bad flag value");
							$$=mk_action(SETFLAG_T, 1, NUMBER_ST,
													(void*)$3);
									}
	| SETFLAG LPAREN flag_name RPAREN	{
							i_tmp=get_flag_no($3, strlen($3));
							if (i_tmp<0) yyerror("flag not declared");
							$$=mk_action(SETFLAG_T, 1, NUMBER_ST,
										(void*)(long)i_tmp);
									}
	| SETFLAG error			{ $$=0; yyerror("missing '(' or ')'?"); }
	| RESETFLAG LPAREN NUMBER RPAREN {
							if (check_flag($3)==-1)
								yyerror("bad flag value");
							$$=mk_action(RESETFLAG_T, 1, NUMBER_ST, (void*)$3);
									}
	| RESETFLAG LPAREN flag_name RPAREN	{
							i_tmp=get_flag_no($3, strlen($3));
							if (i_tmp<0) yyerror("flag not declared");
							$$=mk_action(RESETFLAG_T, 1, NUMBER_ST,
										(void*)(long)i_tmp);
									}
	| RESETFLAG error		{ $$=0; yyerror("missing '(' or ')'?"); }
	| ISFLAGSET LPAREN NUMBER RPAREN {
							if (check_flag($3)==-1)
								yyerror("bad flag value");
							$$=mk_action(ISFLAGSET_T, 1, NUMBER_ST, (void*)$3);
									}
	| ISFLAGSET LPAREN flag_name RPAREN	{
							i_tmp=get_flag_no($3, strlen($3));
							if (i_tmp<0) yyerror("flag not declared");
							$$=mk_action(ISFLAGSET_T, 1, NUMBER_ST,
										(void*)(long)i_tmp);
									}
	| ISFLAGSET error { $$=0; yyerror("missing '(' or ')'?"); }
	| avpflag_oper LPAREN attr_id_any_str COMMA flag_name RPAREN {
		i_tmp=get_avpflag_no($5);
		if (i_tmp==0) yyerror("avpflag not declared");
		$$=mk_action(AVPFLAG_OPER_T, 3, AVP_ST, $3, NUMBER_ST, (void*)(long)i_tmp, NUMBER_ST, (void*)$1);
	}
	| avpflag_oper error { $$=0; yyerror("missing '(' or ')'?"); }
	| ERROR LPAREN STRING COMMA STRING RPAREN {$$=mk_action(ERROR_T, 2, STRING_ST, $3, STRING_ST, $5); }
	| ERROR error { $$=0; yyerror("missing '(' or ')' ?"); }
	| ERROR LPAREN error RPAREN { $$=0; yyerror("bad error argument"); }
	| ROUTE LPAREN route_name RPAREN	{
						i_tmp=route_get(&main_rt, $3);
						if (i_tmp==-1){
							yyerror("internal error");
							YYABORT;
						}
						$$=mk_action(ROUTE_T, 1, NUMBER_ST,(void*)(long)i_tmp);
										}
	| ROUTE error { $$=0; yyerror("missing '(' or ')' ?"); }
	| ROUTE LPAREN error RPAREN { $$=0; yyerror("bad route argument"); }
	| EXEC LPAREN STRING RPAREN	{ $$=mk_action(EXEC_T, 1, STRING_ST, $3); }
	| SET_HOST LPAREN STRING RPAREN { $$=mk_action(SET_HOST_T, 1, STRING_ST, $3); }
	| SET_HOST error { $$=0; yyerror("missing '(' or ')' ?"); }
	| SET_HOST LPAREN error RPAREN { $$=0; yyerror("bad argument, string expected"); }
	| PREFIX LPAREN STRING RPAREN { $$=mk_action(PREFIX_T, 1, STRING_ST,  $3); }
	| PREFIX error { $$=0; yyerror("missing '(' or ')' ?"); }
	| PREFIX LPAREN error RPAREN { $$=0; yyerror("bad argument, string expected"); }
	| STRIP_TAIL LPAREN NUMBER RPAREN { $$=mk_action(STRIP_TAIL_T, 1, NUMBER_ST, (void*)$3); }
	| STRIP_TAIL error { $$=0; yyerror("missing '(' or ')' ?"); }
	| STRIP_TAIL LPAREN error RPAREN { $$=0; yyerror("bad argument, number expected"); }
	| STRIP LPAREN NUMBER RPAREN { $$=mk_action(STRIP_T, 1, NUMBER_ST, (void*) $3); }
	| STRIP error { $$=0; yyerror("missing '(' or ')' ?"); }
	| STRIP LPAREN error RPAREN { $$=0; yyerror("bad argument, number expected"); }
	| SET_USERPHONE LPAREN RPAREN { $$=mk_action(SET_USERPHONE_T, 0); }
	| SET_USERPHONE error { $$=0; yyerror("missing '(' or ')' ?"); }
	| APPEND_BRANCH LPAREN STRING COMMA STRING RPAREN {
		qvalue_t q;
		if (str2q(&q, $5, strlen($5)) < 0) {
			yyerror("bad argument, q value expected");
		}
		$$=mk_action(APPEND_BRANCH_T, 2, STRING_ST, $3, NUMBER_ST, (void *)(long)q);
	}
	| APPEND_BRANCH LPAREN STRING RPAREN { $$=mk_action(APPEND_BRANCH_T, 2, STRING_ST, $3, NUMBER_ST, (void *)Q_UNSPECIFIED); }
	| APPEND_BRANCH LPAREN RPAREN { $$=mk_action(APPEND_BRANCH_T, 2, STRING_ST, 0, NUMBER_ST, (void *)Q_UNSPECIFIED); }
	| APPEND_BRANCH {  $$=mk_action( APPEND_BRANCH_T, 1, STRING_ST, 0); }
	| SET_HOSTPORT LPAREN STRING RPAREN { $$=mk_action(SET_HOSTPORT_T, 1, STRING_ST, $3); }
	| SET_HOSTPORT error { $$=0; yyerror("missing '(' or ')' ?"); }
	| SET_HOSTPORT LPAREN error RPAREN { $$=0; yyerror("bad argument, string expected"); }
	| SET_HOSTPORTTRANS LPAREN STRING RPAREN { $$=mk_action(SET_HOSTPORTTRANS_T, 1, STRING_ST, $3); }
	| SET_HOSTPORTTRANS error { $$=0; yyerror("missing '(' or ')' ?"); }
	| SET_HOSTPORTTRANS LPAREN error RPAREN { $$=0; yyerror("bad argument, string expected"); }
	| SET_PORT LPAREN STRING RPAREN { $$=mk_action(SET_PORT_T, 1, STRING_ST, $3); }
	| SET_PORT error { $$=0; yyerror("missing '(' or ')' ?"); }
	| SET_PORT LPAREN error RPAREN { $$=0; yyerror("bad argument, string expected"); }
	| SET_USER LPAREN STRING RPAREN { $$=mk_action(SET_USER_T, 1, STRING_ST, $3); }
	| SET_USER error { $$=0; yyerror("missing '(' or ')' ?"); }
	| SET_USER LPAREN error RPAREN { $$=0; yyerror("bad argument, string expected"); }
	| SET_USERPASS LPAREN STRING RPAREN { $$=mk_action(SET_USERPASS_T, 1, STRING_ST, $3); }
	| SET_USERPASS error { $$=0; yyerror("missing '(' or ')' ?"); }
	| SET_USERPASS LPAREN error RPAREN { $$=0; yyerror("bad argument, string expected"); }
	| SET_URI LPAREN STRING RPAREN { $$=mk_action(SET_URI_T, 1, STRING_ST,$3); }
	| SET_URI error { $$=0; yyerror("missing '(' or ')' ?"); }
	| SET_URI LPAREN error RPAREN { $$=0; yyerror("bad argument, string expected"); }
	| REVERT_URI LPAREN RPAREN { $$=mk_action(REVERT_URI_T, 0); }
	| REVERT_URI { $$=mk_action(REVERT_URI_T, 0); }
	| FORCE_RPORT LPAREN RPAREN	{ $$=mk_action(FORCE_RPORT_T, 0); }
	| FORCE_RPORT	{$$=mk_action(FORCE_RPORT_T, 0); }
	| FORCE_TCP_ALIAS LPAREN NUMBER RPAREN	{
		#ifdef USE_TCP
			$$=mk_action(FORCE_TCP_ALIAS_T, 1, NUMBER_ST, (void*)$3);
		#else
			yyerror("tcp support not compiled in");
		#endif
	}
	| FORCE_TCP_ALIAS LPAREN RPAREN	{
		#ifdef USE_TCP
			$$=mk_action(FORCE_TCP_ALIAS_T, 0);
		#else
			yyerror("tcp support not compiled in");
		#endif
	}
	| FORCE_TCP_ALIAS				{
		#ifdef USE_TCP
			$$=mk_action(FORCE_TCP_ALIAS_T, 0);
		#else
			yyerror("tcp support not compiled in");
		#endif
	}
	| FORCE_TCP_ALIAS LPAREN error RPAREN	{$$=0; yyerror("bad argument, number expected"); }
	| UDP_MTU_TRY_PROTO LPAREN proto RPAREN
		{ $$=mk_action(UDP_MTU_TRY_PROTO_T, 1, NUMBER_ST, $3); }
	| UDP_MTU_TRY_PROTO LPAREN error RPAREN
		{ $$=0; yyerror("bad argument, UDP, TCP, TLS or SCTP expected"); }
	| SET_ADV_ADDRESS LPAREN listen_id RPAREN {
		$$=0;
		if ((str_tmp=pkg_malloc(sizeof(str)))==0) {
			LOG(L_CRIT, "ERROR: cfg. parser: out of memory.\n");
		} else {
			str_tmp->s=$3;
			str_tmp->len=$3?strlen($3):0;
			$$=mk_action(SET_ADV_ADDR_T, 1, STR_ST, str_tmp);
		}
	}
	| SET_ADV_ADDRESS LPAREN error RPAREN { $$=0; yyerror("bad argument, string expected"); }
	| SET_ADV_ADDRESS error {$$=0; yyerror("missing '(' or ')' ?"); }
	| SET_ADV_PORT LPAREN NUMBER RPAREN {
		$$=0;
		tmp=int2str($3, &i_tmp);
		if ((str_tmp=pkg_malloc(sizeof(str)))==0) {
			LOG(L_CRIT, "ERROR: cfg. parser: out of memory.\n");
		} else {
			if ((str_tmp->s=pkg_malloc(i_tmp))==0) {
				LOG(L_CRIT, "ERROR: cfg. parser: out of memory.\n");
			} else {
				memcpy(str_tmp->s, tmp, i_tmp);
				str_tmp->len=i_tmp;
				$$=mk_action(SET_ADV_PORT_T, 1, STR_ST, str_tmp);
			}
		}
	}
	| SET_ADV_PORT LPAREN error RPAREN { $$=0; yyerror("bad argument, string expected"); }
	| SET_ADV_PORT  error {$$=0; yyerror("missing '(' or ')' ?"); }
	| FORCE_SEND_SOCKET LPAREN phostport RPAREN { 
		$$=mk_action(FORCE_SEND_SOCKET_T, 1, SOCKID_ST, $3);
	}
	| FORCE_SEND_SOCKET LPAREN error RPAREN {
		$$=0; yyerror("bad argument, [proto:]host[:port] expected");
	}
	| FORCE_SEND_SOCKET error {$$=0; yyerror("missing '(' or ')' ?"); }
	| ID {mod_func_action = mk_action(MODULE_T, 2, MODEXP_ST, NULL, NUMBER_ST, 0); } LPAREN func_params RPAREN	{
		mod_func_action->val[0].u.data = find_export_record($1, mod_func_action->val[1].u.number, rt);
		if (mod_func_action->val[0].u.data == 0) {
			if (find_export_record($1, mod_func_action->val[1].u.number, 0) ) {
					yyerror("Command cannot be used in the block\n");
			} else {
				yyerror("unknown command, missing loadmodule?\n");
			}
			pkg_free(mod_func_action);
			mod_func_action=0;
		}
		$$ = mod_func_action;
	}
	;
func_params:
	/* empty */
	| func_params COMMA func_param { }
	| func_param {}
	| func_params error { yyerror("call params error\n"); YYABORT; }
	;
func_param:
        NUMBER {
		if (mod_func_action->val[1].u.number < MAX_ACTIONS-2) {
			mod_func_action->val[mod_func_action->val[1].u.number+2].type = NUMBER_ST;
			mod_func_action->val[mod_func_action->val[1].u.number+2].u.number = $1;
			mod_func_action->val[1].u.number++;
		} else {
			yyerror("Too many arguments\n");
		}
	}
	| STRING {
		if (mod_func_action->val[1].u.number < MAX_ACTIONS-2) {
			mod_func_action->val[mod_func_action->val[1].u.number+2].type = STRING_ST;
			mod_func_action->val[mod_func_action->val[1].u.number+2].u.string = $1;
			mod_func_action->val[1].u.number++;
		} else {
			yyerror("Too many arguments\n");
		}
	}
	;
%%

extern int line;
extern int column;
extern int startcolumn;

static void warn(char* s)
{
	LOG(L_WARN, "cfg. warning: (%d,%d-%d): %s\n", line, startcolumn,
			column, s);
	cfg_warnings++;
}

static void yyerror(char* s)
{
	LOG(L_CRIT, "*** PARSE ERROR *** (%d,%d-%d): %s\n", line, startcolumn,
			column, s);
	cfg_errors++;
}


static struct name_lst* mk_name_lst(char* host, int flags)
{
	struct name_lst* l;
	if (host==0) return 0;
	l=pkg_malloc(sizeof(struct name_lst));
	if (l==0) {
		LOG(L_CRIT,"ERROR: cfg. parser: out of memory.\n");
	} else {
		l->name=host;
		l->flags=flags;
		l->next=0;
	}
	return l;
}


static struct socket_id* mk_listen_id(char* host, int proto, int port)
{
	struct socket_id* l;
	if (host==0) return 0;
	l=pkg_malloc(sizeof(struct socket_id));
	if (l==0) {
		LOG(L_CRIT,"ERROR: cfg. parser: out of memory.\n");
	} else {
		l->addr_lst=mk_name_lst(host, 0);
		if (l->addr_lst==0){
			pkg_free(l);
			return 0;
		}
		l->flags=0;
		l->port=port;
		l->proto=proto;
		l->next=0;
	}
	return l;
}


static void free_name_lst(struct name_lst* lst)
{
	struct name_lst* tmp;
	
	while(lst){
		tmp=lst;
		lst=lst->next;
		pkg_free(tmp);
	}
}


static struct socket_id* mk_listen_id2(struct name_lst* addr_l, int proto,
										int port)
{
	struct socket_id* l;
	if (addr_l==0) return 0;
	l=pkg_malloc(sizeof(struct socket_id));
	if (l==0) {
		LOG(L_CRIT,"ERROR: cfg. parser: out of memory.\n");
	} else {
		l->flags=addr_l->flags;
		l->port=port;
		l->proto=proto;
		l->addr_lst=addr_l;
		l->next=0;
	}
	return l;
}


static void free_socket_id(struct socket_id* i)
{
	free_name_lst(i->addr_lst);
	pkg_free(i);
}


static void free_socket_id_lst(struct socket_id* lst)
{
	struct socket_id* tmp;
	
	while(lst){
		tmp=lst;
		lst=lst->next;
		free_socket_id(tmp);
	}
}

/*
int main(int argc, char ** argv)
{
	if (yyparse()!=0)
		fprintf(stderr, "parsing error\n");
}
*/
