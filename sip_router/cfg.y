/*
 * $Id: cfg.y,v 1.46 2003/03/20 15:40:06 janakj Exp $
 *
 *  cfg grammar
 *
 * Copyright (C) 2001-2003 Fhg Fokus
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
 *
 * History:
 * ---------
 * 2003-01-29  src_port added (jiri)
 * 2003-01-23  mhomed added (jiri)
 * 2003-03-19  replaced all mallocs/frees with pkg_malloc/pkg_free (andrei)
 * 2003-03-19  Added support for route type in find_export (janakj)
 * 2003-03-20  Regex support in modparam (janakj)
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
#include "name_alias.h"

#include "config.h"

#ifdef DEBUG_DMALLOC
#include <dmalloc.h>
#endif

/* hack to avoid alloca usage in the generated C file (needed for compiler
 with no built in alloca, like icc*/
#undef _ALLOCA_H

struct id_list{
	char* s;
	struct id_list* next;
};

extern int yylex();
void yyerror(char* s);
char* tmp;
void* f_tmp;
struct id_list* lst_tmp;
int rt;  /* Type of route block for find_export */
 

%}

%union {
	long intval;
	unsigned long uval;
	char* strval;
	struct expr* expr;
	struct action* action;
	struct net* ipnet;
	struct ip_addr* ipaddr;
	struct id_list* idlst;
}

/* terminals */


/* keywords */
%token FORWARD
%token FORWARD_TCP
%token FORWARD_UDP
%token SEND
%token SEND_TCP
%token DROP
%token LOG_TOK
%token ERROR
%token ROUTE
%token REPL_ROUTE
%token EXEC
%token SET_HOST
%token SET_HOSTPORT
%token PREFIX
%token STRIP
%token APPEND_BRANCH
%token SET_USER
%token SET_USERPASS
%token SET_PORT
%token SET_URI
%token REVERT_URI
%token IF
%token ELSE
%token URIHOST
%token URIPORT
%token MAX_LEN
%token SETFLAG
%token RESETFLAG
%token ISFLAGSET
%token LEN_GT
%token METHOD
%token URI
%token SRCIP
%token SRCPORT
%token DSTIP
%token MYSELF

/* config vars. */
%token DEBUG
%token FORK
%token LOGSTDERROR
%token LISTEN
%token ALIAS
%token DNS
%token REV_DNS
%token PORT
%token STAT
%token CHILDREN
%token CHECK_VIA
%token SYN_BRANCH
%token MEMLOG
%token SIP_WARNING
%token FIFO
%token FIFO_MODE
%token SERVER_SIGNATURE
%token REPLY_TO_VIA
%token LOADMODULE
%token MODPARAM
%token MAXBUFFER
%token USER
%token GROUP
%token MHOMED



/* operators */
%nonassoc EQUAL
%nonassoc EQUAL_T
%nonassoc MATCH
%left OR
%left AND
%left NOT

/* values */
%token <intval> NUMBER
%token <strval> ID
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


/*non-terminals */
%type <expr> exp exp_elem /*, condition*/
%type <action> action actions cmd if_cmd stm
%type <ipaddr> ipv4 ipv6 ip
%type <ipnet> ipnet
%type <strval> host
%type <strval> listen_id
%type <idlst>  id_lst
/*%type <route_el> rules;
  %type <route_el> rule;
*/



%%


cfg:	statements
	;

statements:	statements statement {}
		| statement {}
		| statements error { yyerror(""); YYABORT;}
	;

statement:	assign_stm 
		| module_stm
		| {rt=REQUEST_ROUTE;} route_stm 
		| {rt=REPLY_ROUTE;} reply_route_stm

		| CR	/* null statement*/
	;

listen_id:	ip			{	tmp=ip_addr2a($1);
		 					if(tmp==0){
								LOG(L_CRIT, "ERROR: cfg. parser: bad ip "
										"addresss.\n");
								$$=0;
							}else{
								$$=pkg_malloc(strlen(tmp)+1);
								if ($$==0){
									LOG(L_CRIT, "ERROR: cfg. parser: out of "
											"memory.\n");
								}else{
									strncpy($$, tmp, strlen(tmp)+1);
								}
							}
						}
		 |	ID			{	$$=pkg_malloc(strlen($1)+1);
		 					if ($$==0){
									LOG(L_CRIT, "ERROR: cfg. parser: out of "
											"memory.\n");
							}else{
									strncpy($$, $1, strlen($1)+1);
							}
						}
		 |	STRING			{	$$=pkg_malloc(strlen($1)+1);
		 					if ($$==0){
									LOG(L_CRIT, "ERROR: cfg. parser: out of "
											"memory.\n");
							}else{
									strncpy($$, $1, strlen($1)+1);
							}
						}
		 |	host		{	$$=pkg_malloc(strlen($1)+1);
		 					if ($$==0){
									LOG(L_CRIT, "ERROR: cfg. parser: out of "
											"memory.\n");
							}else{
									strncpy($$, $1, strlen($1)+1);
							}
						}
	;

id_lst:	  listen_id	{	$$=pkg_malloc(sizeof(struct id_list));
						if ($$==0){
							LOG(L_CRIT,"ERROR: cfg. parser: out of memory.\n");
						}else{
							$$->s=$1;
							$$->next=0;
						}
					}
		| listen_id id_lst	{
						$$=pkg_malloc(sizeof(struct id_list));
						if ($$==0){
							LOG(L_CRIT,"ERROR: cfg. parser: out of memory.\n");
						}else{
							$$->s=$1;
							$$->next=$2;
						}
							}
		;


assign_stm:	DEBUG EQUAL NUMBER { debug=$3; }
		| DEBUG EQUAL error  { yyerror("number  expected"); }
		| FORK  EQUAL NUMBER { dont_fork= ! $3; }
		| FORK  EQUAL error  { yyerror("boolean value expected"); }
		| LOGSTDERROR EQUAL NUMBER { log_stderr=$3; }
		| LOGSTDERROR EQUAL error { yyerror("boolean value expected"); }
		| DNS EQUAL NUMBER   { received_dns|= ($3)?DO_DNS:0; }
		| DNS EQUAL error { yyerror("boolean value expected"); }
		| REV_DNS EQUAL NUMBER { received_dns|= ($3)?DO_REV_DNS:0; }
		| REV_DNS EQUAL error { yyerror("boolean value expected"); }
		| PORT EQUAL NUMBER   { port_no=$3; 
								if (sock_no>0) 
									sock_info[sock_no-1].port_no=port_no;
							  }
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
		| SYN_BRANCH EQUAL NUMBER { syn_branch=$3; }
		| SYN_BRANCH EQUAL error { yyerror("boolean value expected"); }
		| MEMLOG EQUAL NUMBER { memlog=$3; }
		| MEMLOG EQUAL error { yyerror("int value expected"); }
		| SIP_WARNING EQUAL NUMBER { sip_warning=$3; }
		| SIP_WARNING EQUAL error { yyerror("boolean value expected"); }
		| FIFO EQUAL STRING { fifo=$3; }
		| FIFO EQUAL error { yyerror("string value expected"); }
		| FIFO_MODE EQUAL NUMBER { fifo_mode=$3; }
		| FIFO_MODE EQUAL error { yyerror("int value expected"); }
		| USER EQUAL STRING     { user=$3; }
		| USER EQUAL ID         { user=$3; }
		| USER EQUAL error      { yyerror("string value expected"); }
		| GROUP EQUAL STRING     { group=$3; }
		| GROUP EQUAL ID         { group=$3; }
		| GROUP EQUAL error      { yyerror("string value expected"); }
		| MHOMED EQUAL NUMBER { mhomed=$3; }
		| MHOMED EQUAL error { yyerror("boolean value expected"); }
		| SERVER_SIGNATURE EQUAL NUMBER { server_signature=$3; }
		| SERVER_SIGNATURE EQUAL error { yyerror("boolean value expected"); }
		| REPLY_TO_VIA EQUAL NUMBER { reply_to_via=$3; }
		| REPLY_TO_VIA EQUAL error { yyerror("boolean value expected"); }
		| LISTEN EQUAL id_lst {
							for(lst_tmp=$3; lst_tmp; lst_tmp=lst_tmp->next){
								if (sock_no < MAX_LISTEN){
									sock_info[sock_no].name.s=(char*)
											pkg_malloc(strlen(lst_tmp->s)+1);
									if (sock_info[sock_no].name.s==0){
										LOG(L_CRIT, "ERROR: cfg. parser:"
													" out of memory.\n");
										break;
									}else{
										strncpy(sock_info[sock_no].name.s,
												lst_tmp->s,
												strlen(lst_tmp->s)+1);
										sock_info[sock_no].name.len=
													strlen(lst_tmp->s);
										sock_info[sock_no].port_no=port_no;
										sock_no++;
									}
								}else{
									LOG(L_CRIT, "ERROR: cfg. parser: "
												"too many listen addresses"
												"(max. %d).\n", MAX_LISTEN);
									break;
								}
							}
							 }
		| LISTEN EQUAL  error { yyerror("ip address or hostname"
						"expected"); }
		| ALIAS EQUAL  id_lst { 
							for(lst_tmp=$3; lst_tmp; lst_tmp=lst_tmp->next)
								add_alias(lst_tmp->s, strlen(lst_tmp->s), 0);
							  }
		| ALIAS  EQUAL error  { yyerror(" hostname expected"); }
		| error EQUAL { yyerror("unknown config variable"); }
	;

module_stm:	LOADMODULE STRING	{ DBG("loading module %s\n", $2);
		  						  if (load_module($2)!=0){
								  		yyerror("failed to load module");
								  }
								}
		 | LOADMODULE error	{ yyerror("string expected");  }
                 | MODPARAM LPAREN STRING COMMA STRING COMMA STRING RPAREN {
			 if (set_mod_param_regex($3, $5, STR_PARAM, $7) != 0) {
				 yyerror("Can't set module parameter");
			 }
		   }
                 | MODPARAM LPAREN STRING COMMA STRING COMMA NUMBER RPAREN {
			 if (set_mod_param_regex($3, $5, INT_PARAM, (void*)$7) != 0) {
				 yyerror("Can't set module parameter");
			 }
		   }
                 | MODPARAM error { yyerror("Invalid arguments"); }
		 ;


ip:		 ipv4  { $$=$1; }
		|ipv6  { $$=$1; }
		;

ipv4:	NUMBER DOT NUMBER DOT NUMBER DOT NUMBER { 
											$$=pkg_malloc(
													sizeof(struct ip_addr));
											if ($$==0){
												LOG(L_CRIT, "ERROR: cfg. "
													"parser: out of memory.\n"
													);
											}else{
												memset($$, 0, 
													sizeof(struct ip_addr));
												$$->af=AF_INET;
												$$->len=4;
												if (($1>255) || ($1<0) ||
													($3>255) || ($3<0) ||
													($5>255) || ($5<0) ||
													($7>255) || ($7<0)){
													yyerror("invalid ipv4"
															"address");
													$$->u.addr32[0]=0;
													/* $$=0; */
												}else{
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

ipv6:	IPV6ADDR {
					$$=pkg_malloc(sizeof(struct ip_addr));
					if ($$==0){
						LOG(L_CRIT, "ERROR: cfg. parser: out of memory.\n");
					}else{
						memset($$, 0, sizeof(struct ip_addr));
						$$->af=AF_INET6;
						$$->len=16;
					#ifdef USE_IPV6
						if (inet_pton(AF_INET6, $1, $$->u.addr)<=0){
							yyerror("bad ipv6 address");
						}
					#else
						yyerror("ipv6 address & no ipv6 support compiled in");
						YYABORT;
					#endif
					}
				}
	;


route_stm:  ROUTE LBRACE actions RBRACE { push($3, &rlist[DEFAULT_RT]); }

	    | ROUTE LBRACK NUMBER RBRACK LBRACE actions RBRACE { 
										if (($3<RT_NO) && ($3>=0)){
											push($6, &rlist[$3]);
										}else{
											yyerror("invalid routing"
													"table number");
											YYABORT; }
										}
		| ROUTE error { yyerror("invalid  route  statement"); }
	;

reply_route_stm: REPL_ROUTE LBRACK NUMBER RBRACK LBRACE actions RBRACE {
										if (($3<REPLY_RT_NO)&&($3>=1)){
											push($6, &reply_rlist[$3]);
										} else {
											yyerror("invalid reply routing"
												"table number");
											YYABORT; }
										}
		| REPL_ROUTE error { yyerror("invalid reply_route statement"); }
	;
/*
rules:	rules rule { push($2, &$1); $$=$1; }
	| rule {$$=$1; }
	| rules error { $$=0; yyerror("invalid rule"); }
	 ;

rule:	condition	actions CR {
								$$=0;
								if (add_rule($1, $2, &$$)<0) {
									yyerror("error calling add_rule");
									YYABORT;
								}
							  }
	| CR		{ $$=0;}
	| condition error { $$=0; yyerror("bad actions in rule"); }
	;

condition:	exp {$$=$1;}
*/

exp:	exp AND exp 	{ $$=mk_exp(AND_OP, $1, $3); }
	| exp OR  exp		{ $$=mk_exp(OR_OP, $1, $3);  }
	| NOT exp 			{ $$=mk_exp(NOT_OP, $2, 0);  }
	| LPAREN exp RPAREN	{ $$=$2; }
	| exp_elem			{ $$=$1; }
	;

exp_elem:	METHOD EQUAL_T STRING	{$$= mk_elem(	EQUAL_OP, STRING_ST, 
													METHOD_O, $3);
									}
		| METHOD EQUAL_T ID	{$$ = mk_elem(	EQUAL_OP, STRING_ST,
											METHOD_O, $3); 
				 			}
		| METHOD EQUAL_T error { $$=0; yyerror("string expected"); }
		| METHOD MATCH STRING	{$$ = mk_elem(	MATCH_OP, STRING_ST,
												METHOD_O, $3); 
				 				}
		| METHOD MATCH ID	{$$ = mk_elem(	MATCH_OP, STRING_ST,
											METHOD_O, $3); 
				 			}
		| METHOD MATCH error { $$=0; yyerror("string expected"); }
		| METHOD error	{ $$=0; yyerror("invalid operator,"
										"== or =~ expected");
						}
		| URI EQUAL_T STRING 	{$$ = mk_elem(	EQUAL_OP, STRING_ST,
												URI_O, $3); 
				 				}
		| URI EQUAL_T ID 	{$$ = mk_elem(	EQUAL_OP, STRING_ST,
											URI_O, $3); 
				 			}
		| URI EQUAL_T MYSELF    { $$=mk_elem(	EQUAL_OP, MYSELF_ST,
												URI_O, 0);
								}
		| URI EQUAL_T error { $$=0; yyerror("string expected"); }
		| URI MATCH STRING	{ $$=mk_elem(	MATCH_OP, STRING_ST,
											URI_O, $3);
							}
		| URI MATCH ID		{ $$=mk_elem(	MATCH_OP, STRING_ST,
											URI_O, $3);
							}
		| URI MATCH error {  $$=0; yyerror("string expected"); }
		| URI error	{ $$=0; yyerror("invalid operator,"
				  					" == or =~ expected");
					}
		| SRCPORT EQUAL_T NUMBER	{ $$=mk_elem(	EQUAL_OP, NUMBER_ST,
												SRCPORT_O, (void *) $3 ); }
		| SRCPORT EQUAL_T error { $$=0; yyerror("number expected"); }
		| SRCPORT error { $$=0; yyerror("equation operator expected"); }
		| SRCIP EQUAL_T ipnet	{ $$=mk_elem(	EQUAL_OP, NET_ST,
												SRCIP_O, $3);
								}
		| SRCIP EQUAL_T STRING	{ $$=mk_elem(	EQUAL_OP, STRING_ST,
												SRCIP_O, $3);
								}
		| SRCIP EQUAL_T host	{ $$=mk_elem(	EQUAL_OP, STRING_ST,
												SRCIP_O, $3);
								}
		| SRCIP EQUAL_T MYSELF  { $$=mk_elem(	EQUAL_OP, MYSELF_ST,
												SRCIP_O, 0);
								}
		| SRCIP EQUAL_T error { $$=0; yyerror( "ip address or hostname"
						 "expected" ); }
		| SRCIP MATCH STRING	{ $$=mk_elem(	MATCH_OP, STRING_ST,
												SRCIP_O, $3);
								}
		| SRCIP MATCH ID		{ $$=mk_elem(	MATCH_OP, STRING_ST,
												SRCIP_O, $3);
								}
		| SRCIP MATCH error  { $$=0; yyerror( "hostname expected"); }
		| SRCIP error  { $$=0; 
						 yyerror("invalid operator, == or =~ expected");}
		| DSTIP EQUAL_T ipnet	{ $$=mk_elem(	EQUAL_OP, NET_ST,
												DSTIP_O, $3);
								}
		| DSTIP EQUAL_T STRING	{ $$=mk_elem(	EQUAL_OP, STRING_ST,
												DSTIP_O, $3);
								}
		| DSTIP EQUAL_T host	{ $$=mk_elem(	EQUAL_OP, STRING_ST,
												DSTIP_O, $3);
								}
		| DSTIP EQUAL_T MYSELF  { $$=mk_elem(	EQUAL_OP, MYSELF_ST,
												DSTIP_O, 0);
								}
		| DSTIP EQUAL_T error { $$=0; yyerror( "ip address or hostname"
						 			"expected" ); }
		| DSTIP MATCH STRING	{ $$=mk_elem(	MATCH_OP, STRING_ST,
												DSTIP_O, $3);
								}
		| DSTIP MATCH ID	{ $$=mk_elem(	MATCH_OP, STRING_ST,
											DSTIP_O, $3);
							}
		| DSTIP MATCH error  { $$=0; yyerror ( "hostname  expected" ); }
		| DSTIP error { $$=0; 
						yyerror("invalid operator, == or =~ expected");}
		| MYSELF EQUAL_T URI    { $$=mk_elem(	EQUAL_OP, MYSELF_ST,
												URI_O, 0);
								}
		| MYSELF EQUAL_T SRCIP  { $$=mk_elem(	EQUAL_OP, MYSELF_ST,
												SRCIP_O, 0);
								}
		| MYSELF EQUAL_T DSTIP  { $$=mk_elem(	EQUAL_OP, MYSELF_ST,
												DSTIP_O, 0);
								}
		| MYSELF EQUAL_T error {	$$=0; 
									yyerror(" URI, SRCIP or DSTIP expected"); }
		| MYSELF error	{ $$=0; yyerror ("invalid operator, == expected"); }
		| stm				{ $$=mk_elem( NO_OP, ACTIONS_ST, ACTION_O, $1 ); }
		| NUMBER		{$$=mk_elem( NO_OP, NUMBER_ST, NUMBER_O, (void*)$1 ); }
	;

ipnet:	ip SLASH ip	{ $$=mk_net($1, $3); } 
	| ip SLASH NUMBER 	{	if (($3<0) || ($3>$1->len*8)){
								yyerror("invalid bit number in netmask");
								$$=0;
							}else{
								$$=mk_net_bitlen($1, $3);
							/*
								$$=mk_net($1, 
										htonl( ($3)?~( (1<<(32-$3))-1 ):0 ) );
							*/
							}
						}
	| ip				{ $$=mk_net_bitlen($1, $1->len*8); }
	| ip SLASH error	{ $$=0;
						 yyerror("netmask (eg:255.0.0.0 or 8) expected");
						}
	;

host:	ID				{ $$=$1; }
	| host DOT ID		{ $$=(char*)pkg_malloc(strlen($1)+1+strlen($3)+1);
						  if ($$==0){
						  	LOG(L_CRIT, "ERROR: cfg. parser: memory allocation"
										" failure while parsing host\n");
						  }else{
						  	memcpy($$, $1, strlen($1));
						  	$$[strlen($1)]='.';
						  	memcpy($$+strlen($1)+1, $3, strlen($3));
						  	$$[strlen($1)+1+strlen($3)]=0;
						  }
						  pkg_free($1); pkg_free($3);
						}
	| host DOT error { $$=0; pkg_free($1); yyerror("invalid hostname"); }
	;


stm:		cmd						{ $$=$1; }
		|	LBRACE actions RBRACE	{ $$=$2; }
	;

actions:	actions action	{$$=append_action($1, $2); }
		| action			{$$=$1;}
		| actions error { $$=0; yyerror("bad command"); }
	;

action:		cmd SEMICOLON {$$=$1;}
		| SEMICOLON /* null action */ {$$=0;}
		| cmd error { $$=0; yyerror("bad command: missing ';'?"); }
	;

if_cmd:		IF exp stm				{ $$=mk_action3( IF_T,
													 EXPR_ST,
													 ACTIONS_ST,
													 NOSUBTYPE,
													 $2,
													 $3,
													 0);
									}
		|	IF exp stm ELSE stm		{ $$=mk_action3( IF_T,
													 EXPR_ST,
													 ACTIONS_ST,
													 ACTIONS_ST,
													 $2,
													 $3,
													 $5);
									}
	;

cmd:		FORWARD LPAREN host RPAREN	{ $$=mk_action(	FORWARD_T,
														STRING_ST,
														NUMBER_ST,
														$3,
														0);
										}
		| FORWARD LPAREN STRING RPAREN	{ $$=mk_action(	FORWARD_T,
														STRING_ST,
														NUMBER_ST,
														$3,
														0);
										}
		| FORWARD LPAREN ip RPAREN	{ $$=mk_action(	FORWARD_T,
														IP_ST,
														NUMBER_ST,
														(void*)$3,
														0);
										}
		| FORWARD LPAREN host COMMA NUMBER RPAREN { $$=mk_action(FORWARD_T,
																 STRING_ST,
																 NUMBER_ST,
																$3,
																(void*)$5);
												 }
		| FORWARD LPAREN STRING COMMA NUMBER RPAREN {$$=mk_action(FORWARD_T,
																 STRING_ST,
																 NUMBER_ST,
																$3,
																(void*)$5);
													}
		| FORWARD LPAREN ip COMMA NUMBER RPAREN { $$=mk_action(FORWARD_T,
																 IP_ST,
																 NUMBER_ST,
																 (void*)$3,
																(void*)$5);
												  }
		| FORWARD LPAREN URIHOST COMMA URIPORT RPAREN {
													$$=mk_action(FORWARD_T,
																 URIHOST_ST,
																 URIPORT_ST,
																0,
																0);
													}
													
									
		| FORWARD LPAREN URIHOST COMMA NUMBER RPAREN {
													$$=mk_action(FORWARD_T,
																 URIHOST_ST,
																 NUMBER_ST,
																0,
																(void*)$5);
													}
		| FORWARD LPAREN URIHOST RPAREN {
													$$=mk_action(FORWARD_T,
																 URIHOST_ST,
																 NUMBER_ST,
																0,
																0);
										}
		| FORWARD error { $$=0; yyerror("missing '(' or ')' ?"); }
		| FORWARD LPAREN error RPAREN { $$=0; yyerror("bad forward"
										"argument"); }
		| FORWARD_UDP LPAREN host RPAREN	{ $$=mk_action(	FORWARD_UDP_T,
														STRING_ST,
														NUMBER_ST,
														$3,
														0);
										}
		| FORWARD_UDP LPAREN STRING RPAREN	{ $$=mk_action(	FORWARD_UDP_T,
														STRING_ST,
														NUMBER_ST,
														$3,
														0);
										}
		| FORWARD_UDP LPAREN ip RPAREN	{ $$=mk_action(	FORWARD_UDP_T,
														IP_ST,
														NUMBER_ST,
														(void*)$3,
														0);
										}
		| FORWARD_UDP LPAREN host COMMA NUMBER RPAREN { $$=mk_action(
																FORWARD_UDP_T,
																 STRING_ST,
																 NUMBER_ST,
																$3,
																(void*)$5);
												 }
		| FORWARD_UDP LPAREN STRING COMMA NUMBER RPAREN {$$=mk_action(
																FORWARD_UDP_T,
																 STRING_ST,
																 NUMBER_ST,
																$3,
																(void*)$5);
													}
		| FORWARD_UDP LPAREN ip COMMA NUMBER RPAREN { $$=mk_action(
																FORWARD_UDP_T,
																 IP_ST,
																 NUMBER_ST,
																 (void*)$3,
																(void*)$5);
												  }
		| FORWARD_UDP LPAREN URIHOST COMMA URIPORT RPAREN {
													$$=mk_action(FORWARD_UDP_T,
																 URIHOST_ST,
																 URIPORT_ST,
																0,
																0);
													}
													
									
		| FORWARD_UDP LPAREN URIHOST COMMA NUMBER RPAREN {
													$$=mk_action(FORWARD_UDP_T,
																 URIHOST_ST,
																 NUMBER_ST,
																0,
																(void*)$5);
													}
		| FORWARD_UDP LPAREN URIHOST RPAREN {
													$$=mk_action(FORWARD_UDP_T,
																 URIHOST_ST,
																 NUMBER_ST,
																0,
																0);
										}
		| FORWARD_UDP error { $$=0; yyerror("missing '(' or ')' ?"); }
		| FORWARD_UDP LPAREN error RPAREN { $$=0; yyerror("bad forward_udp"
										"argument"); }
		| FORWARD_TCP LPAREN host RPAREN	{ $$=mk_action(	FORWARD_TCP_T,
														STRING_ST,
														NUMBER_ST,
														$3,
														0);
										}
		| FORWARD_TCP LPAREN STRING RPAREN	{ $$=mk_action(	FORWARD_TCP_T,
														STRING_ST,
														NUMBER_ST,
														$3,
														0);
										}
		| FORWARD_TCP LPAREN ip RPAREN	{ $$=mk_action(	FORWARD_TCP_T,
														IP_ST,
														NUMBER_ST,
														(void*)$3,
														0);
										}
		| FORWARD_TCP LPAREN host COMMA NUMBER RPAREN { $$=mk_action(
																FORWARD_TCP_T,
																 STRING_ST,
																 NUMBER_ST,
																$3,
																(void*)$5);
												 }
		| FORWARD_TCP LPAREN STRING COMMA NUMBER RPAREN {$$=mk_action(
																FORWARD_TCP_T,
																 STRING_ST,
																 NUMBER_ST,
																$3,
																(void*)$5);
													}
		| FORWARD_TCP LPAREN ip COMMA NUMBER RPAREN { $$=mk_action(FORWARD_TCP_T,
																 IP_ST,
																 NUMBER_ST,
																 (void*)$3,
																(void*)$5);
												  }
		| FORWARD_TCP LPAREN URIHOST COMMA URIPORT RPAREN {
													$$=mk_action(FORWARD_TCP_T,
																 URIHOST_ST,
																 URIPORT_ST,
																0,
																0);
													}
													
									
		| FORWARD_TCP LPAREN URIHOST COMMA NUMBER RPAREN {
													$$=mk_action(FORWARD_TCP_T,
																 URIHOST_ST,
																 NUMBER_ST,
																0,
																(void*)$5);
													}
		| FORWARD_TCP LPAREN URIHOST RPAREN {
													$$=mk_action(FORWARD_TCP_T,
																 URIHOST_ST,
																 NUMBER_ST,
																0,
																0);
										}
		| FORWARD_TCP error { $$=0; yyerror("missing '(' or ')' ?"); }
		| FORWARD_TCP LPAREN error RPAREN { $$=0; yyerror("bad forward_tcp"
										"argument"); }
		| SEND LPAREN host RPAREN	{ $$=mk_action(	SEND_T,
													STRING_ST,
													NUMBER_ST,
													$3,
													0);
									}
		| SEND LPAREN STRING RPAREN { $$=mk_action(	SEND_T,
													STRING_ST,
													NUMBER_ST,
													$3,
													0);
									}
		| SEND LPAREN ip RPAREN		{ $$=mk_action(	SEND_T,
													IP_ST,
													NUMBER_ST,
													(void*)$3,
													0);
									}
		| SEND LPAREN host COMMA NUMBER RPAREN	{ $$=mk_action(	SEND_T,
																STRING_ST,
																NUMBER_ST,
																$3,
																(void*)$5);
												}
		| SEND LPAREN STRING COMMA NUMBER RPAREN {$$=mk_action(	SEND_T,
																STRING_ST,
																NUMBER_ST,
																$3,
																(void*)$5);
												}
		| SEND LPAREN ip COMMA NUMBER RPAREN { $$=mk_action(	SEND_T,
																IP_ST,
																NUMBER_ST,
																(void*)$3,
																(void*)$5);
											   }
		| SEND error { $$=0; yyerror("missing '(' or ')' ?"); }
		| SEND LPAREN error RPAREN { $$=0; yyerror("bad send"
													"argument"); }
		| SEND_TCP LPAREN host RPAREN	{ $$=mk_action(	SEND_TCP_T,
													STRING_ST,
													NUMBER_ST,
													$3,
													0);
									}
		| SEND_TCP LPAREN STRING RPAREN { $$=mk_action(	SEND_TCP_T,
													STRING_ST,
													NUMBER_ST,
													$3,
													0);
									}
		| SEND_TCP LPAREN ip RPAREN		{ $$=mk_action(	SEND_TCP_T,
													IP_ST,
													NUMBER_ST,
													(void*)$3,
													0);
									}
		| SEND_TCP LPAREN host COMMA NUMBER RPAREN	{ $$=mk_action(	SEND_TCP_T,
																STRING_ST,
																NUMBER_ST,
																$3,
																(void*)$5);
												}
		| SEND_TCP LPAREN STRING COMMA NUMBER RPAREN {$$=mk_action(	SEND_TCP_T,
																STRING_ST,
																NUMBER_ST,
																$3,
																(void*)$5);
												}
		| SEND_TCP LPAREN ip COMMA NUMBER RPAREN { $$=mk_action(	SEND_TCP_T,
																IP_ST,
																NUMBER_ST,
																(void*)$3,
																(void*)$5);
											   }
		| SEND_TCP error { $$=0; yyerror("missing '(' or ')' ?"); }
		| SEND_TCP LPAREN error RPAREN { $$=0; yyerror("bad send_tcp"
													"argument"); }
		| DROP LPAREN RPAREN	{$$=mk_action(DROP_T,0, 0, 0, 0); }
		| DROP					{$$=mk_action(DROP_T,0, 0, 0, 0); }
		| LOG_TOK LPAREN STRING RPAREN	{$$=mk_action(	LOG_T, NUMBER_ST, 
													STRING_ST,(void*)4,$3);
									}
		| LOG_TOK LPAREN NUMBER COMMA STRING RPAREN	{$$=mk_action(	LOG_T,
																NUMBER_ST, 
																STRING_ST,
																(void*)$3,
																$5);
												}
		| LOG_TOK error { $$=0; yyerror("missing '(' or ')' ?"); }
		| LOG_TOK LPAREN error RPAREN { $$=0; yyerror("bad log"
									"argument"); }
		| SETFLAG LPAREN NUMBER RPAREN {$$=mk_action( SETFLAG_T, NUMBER_ST, 0,
													(void *)$3, 0 ); }
		| SETFLAG error { $$=0; yyerror("missing '(' or ')'?"); }

		| LEN_GT LPAREN NUMBER RPAREN {$$=mk_action( LEN_GT_T, NUMBER_ST, 0,
													(void *)$3, 0 ); }
		| LEN_GT LPAREN MAX_LEN RPAREN {$$=mk_action( LEN_GT_T, NUMBER_ST, 0,
													(void *) BUF_SIZE, 0 ); }
		| LEN_GT error { $$=0; yyerror("missing '(' or ')'?"); }

		| RESETFLAG LPAREN NUMBER RPAREN {$$=mk_action(	RESETFLAG_T, NUMBER_ST, 0,
													(void *)$3, 0 ); }
		| RESETFLAG error { $$=0; yyerror("missing '(' or ')'?"); }
		| ISFLAGSET LPAREN NUMBER RPAREN {$$=mk_action(	ISFLAGSET_T, NUMBER_ST, 0,
													(void *)$3, 0 ); }
		| ISFLAGSET error { $$=0; yyerror("missing '(' or ')'?"); }
		| ERROR LPAREN STRING COMMA STRING RPAREN {$$=mk_action(ERROR_T,
																STRING_ST, 
																STRING_ST,
																$3,
																$5);
												  }
		| ERROR error { $$=0; yyerror("missing '(' or ')' ?"); }
		| ERROR LPAREN error RPAREN { $$=0; yyerror("bad error"
														"argument"); }
		| ROUTE LPAREN NUMBER RPAREN	{ $$=mk_action(ROUTE_T, NUMBER_ST,
														0, (void*)$3, 0);
										}
		| ROUTE error { $$=0; yyerror("missing '(' or ')' ?"); }
		| ROUTE LPAREN error RPAREN { $$=0; yyerror("bad route"
						"argument"); }
		| EXEC LPAREN STRING RPAREN	{ $$=mk_action(	EXEC_T, STRING_ST, 0,
													$3, 0);
									}
		| SET_HOST LPAREN STRING RPAREN { $$=mk_action(SET_HOST_T, STRING_ST,
														0, $3, 0); }
		| SET_HOST error { $$=0; yyerror("missing '(' or ')' ?"); }
		| SET_HOST LPAREN error RPAREN { $$=0; yyerror("bad argument, "
														"string expected"); }

		| PREFIX LPAREN STRING RPAREN { $$=mk_action(PREFIX_T, STRING_ST,
														0, $3, 0); }
		| PREFIX error { $$=0; yyerror("missing '(' or ')' ?"); }
		| PREFIX LPAREN error RPAREN { $$=0; yyerror("bad argument, "
														"string expected"); }
		| STRIP LPAREN NUMBER RPAREN { $$=mk_action(STRIP_T, NUMBER_ST,
														0, (void *) $3, 0); }
		| STRIP error { $$=0; yyerror("missing '(' or ')' ?"); }
		| STRIP LPAREN error RPAREN { $$=0; yyerror("bad argument, "
														"number expected"); }

		| APPEND_BRANCH LPAREN STRING RPAREN { $$=mk_action( APPEND_BRANCH_T,
													STRING_ST, 0, $3, 0) ; }
		| APPEND_BRANCH LPAREN RPAREN { $$=mk_action( APPEND_BRANCH_T,
													STRING_ST, 0, 0, 0 ) ; }
		| APPEND_BRANCH {  $$=mk_action( APPEND_BRANCH_T, STRING_ST, 0, 0, 0 ) ; }

		| SET_HOSTPORT LPAREN STRING RPAREN { $$=mk_action( SET_HOSTPORT_T, 
														STRING_ST, 0, $3, 0); }
		| SET_HOSTPORT error { $$=0; yyerror("missing '(' or ')' ?"); }
		| SET_HOSTPORT LPAREN error RPAREN { $$=0; yyerror("bad argument,"
												" string expected"); }
		| SET_PORT LPAREN STRING RPAREN { $$=mk_action( SET_PORT_T, STRING_ST,
														0, $3, 0); }
		| SET_PORT error { $$=0; yyerror("missing '(' or ')' ?"); }
		| SET_PORT LPAREN error RPAREN { $$=0; yyerror("bad argument, "
														"string expected"); }
		| SET_USER LPAREN STRING RPAREN { $$=mk_action( SET_USER_T, STRING_ST,
														0, $3, 0); }
		| SET_USER error { $$=0; yyerror("missing '(' or ')' ?"); }
		| SET_USER LPAREN error RPAREN { $$=0; yyerror("bad argument, "
														"string expected"); }
		| SET_USERPASS LPAREN STRING RPAREN { $$=mk_action( SET_USERPASS_T, 
														STRING_ST, 0, $3, 0); }
		| SET_USERPASS error { $$=0; yyerror("missing '(' or ')' ?"); }
		| SET_USERPASS LPAREN error RPAREN { $$=0; yyerror("bad argument, "
														"string expected"); }
		| SET_URI LPAREN STRING RPAREN { $$=mk_action( SET_URI_T, STRING_ST, 
														0, $3, 0); }
		| SET_URI error { $$=0; yyerror("missing '(' or ')' ?"); }
		| SET_URI LPAREN error RPAREN { $$=0; yyerror("bad argument, "
										"string expected"); }
		| REVERT_URI LPAREN RPAREN { $$=mk_action( REVERT_URI_T, 0,0,0,0); }
		| REVERT_URI { $$=mk_action( REVERT_URI_T, 0,0,0,0); }
		| ID LPAREN RPAREN			{ f_tmp=(void*)find_export($1, 0, rt);
									   if (f_tmp==0){
										   if (find_export($1, 0, 0)) {
											   yyerror("Command cannot be used in the block\n");
										   } else {
											   yyerror("unknown command, missing"
												   " loadmodule?\n");
										   }
										$$=0;
									   }else{
										$$=mk_action(	MODULE_T,
														CMDF_ST,
														0,
														f_tmp,
														0
													);
									   }
									}
		| ID LPAREN STRING RPAREN { f_tmp=(void*)find_export($1, 1, rt);
									if (f_tmp==0){
										if (find_export($1, 1, 0)) {
											yyerror("Command cannot be used in the block\n");
										} else {
											yyerror("unknown command, missing"
												" loadmodule?\n");
										}
										$$=0;
									}else{
										$$=mk_action(	MODULE_T,
														CMDF_ST,
														STRING_ST,
														f_tmp,
														$3
													);
									}
								  }
		| ID LPAREN STRING  COMMA STRING RPAREN 
								  { f_tmp=(void*)find_export($1, 2, rt);
									if (f_tmp==0){
										if (find_export($1, 2, 0)) {
											yyerror("Command cannot be used in the block\n");
										} else {
											yyerror("unknown command, missing"
												" loadmodule?\n");
										}
										$$=0;
									}else{
										$$=mk_action3(	MODULE_T,
														CMDF_ST,
														STRING_ST,
														STRING_ST,
														f_tmp,
														$3,
														$5
													);
									}
								  }
		| ID LPAREN error RPAREN { $$=0; yyerror("bad arguments"); }
		| if_cmd		{ $$=$1; }
	;


%%

extern int line;
extern int column;
extern int startcolumn;
void yyerror(char* s)
{
	LOG(L_CRIT, "parse error (%d,%d-%d): %s\n", line, startcolumn, 
			column, s);
	cfg_errors++;
}

/*
int main(int argc, char ** argv)
{
	if (yyparse()!=0)
		fprintf(stderr, "parsing error\n");
}
*/
