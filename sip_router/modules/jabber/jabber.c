/*
 * $Id: jabber.c,v 1.28 2003/01/16 18:43:22 dcm Exp $
 *
 * XJAB module
 *
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
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "../../sr_module.h"
#include "../../error.h"
#include "../../ut.h"
#include "../../mem/shm_mem.h"
#include "../../mem/mem.h"
#include "../../globals.h"
#include "../../timer.h"
#include "../../parser/parse_uri.h"
#include "../../parser/parse_content.h"
#include "../../parser/parse_from.h"
#include "../../db/db.h"

#include "../tm/tm_load.h"

#include "xjab_load.h"
#include "xjab_worker.h"
#include "xjab_util.h"


/** TM bind */
struct tm_binds tmb;

/** workers list */
xj_wlist jwl = NULL;

/** Structure that represents database connection */
db_con_t** db_con;

/** parameters */

char *db_url   = "sql://root@127.0.0.1/sip_jab";
char *db_table = "jusers";

int nrw = 2;
int max_jobs = 10;

char *contact = "-";
char *jaddress = "127.0.0.1";
int jport = 5222;

char *jaliases = NULL;
char *jdomain  = NULL;

int delay_time = 90;
int sleep_time = 20;
int cache_time = 600;
int check_time = 20;

int **pipes = NULL;

static int mod_init(void);
static int child_init(int rank);

int xjab_manage_sipmsg(struct sip_msg *msg, int type);
void xjab_check_workers(int mpid);

static int xj_send_message(struct sip_msg*, char*, char*);
static int xj_join_jconf(struct sip_msg*, char*, char*);
static int xj_exit_jconf(struct sip_msg*, char*, char*);
static int xj_go_online(struct sip_msg*, char*, char*);
static int xj_go_offline(struct sip_msg*, char*, char*);

void destroy(void);

struct module_exports exports= {
	"jabber",
	(char*[]){
		"jab_send_message",
		"jab_join_jconf",
		"jab_exit_jconf",
		"jab_go_online",
		"jab_go_offline",
		"jab_register_watcher",
		"jab_unregister_watcher",
		"load_xjab"
	},
	(cmd_function[]){
		xj_send_message,
		xj_join_jconf,
		xj_exit_jconf,
		xj_go_online,
		xj_go_offline,
		(cmd_function)xj_register_watcher,
		(cmd_function)xj_unregister_watcher,
		(cmd_function)load_xjab
	},
	(int[]){
		0, 0, 0, 0, 0,
		XJ_NO_SCRIPT_F,
		XJ_NO_SCRIPT_F,
		XJ_NO_SCRIPT_F
	},
	(fixup_function[]){
		0, 0, 0, 0, 0,
		0,
		0,
		0
	},
	8,

	(char*[]) {   /* Module parameter names */
		"contact",
		"db_url",
		"jaddress",
		"aliases",
		"jdomain",
		"jport",
		"workers",
		"max_jobs",
		"cache_time",
		"delay_time",
		"sleep_time",
		"check_time"
	},
	(modparam_t[]) {   /* Module parameter types */
		STR_PARAM,
		STR_PARAM,
		STR_PARAM,
		STR_PARAM,
		STR_PARAM,
		INT_PARAM,
		INT_PARAM,
		INT_PARAM,
		INT_PARAM,
		INT_PARAM,
		INT_PARAM
	},
	(void*[]) {   /* Module parameter variable pointers */
		&contact,
		&db_url,
		&jaddress,
		&jaliases,
		&jdomain,
		&jport,
		&nrw,
		&max_jobs,
		&cache_time,
		&delay_time,
		&sleep_time,
		&check_time
	},
	12,      /* Number of module paramers */
	
	mod_init,   /* module initialization function */
	(response_function) 0,
	(destroy_function) destroy,
	0,
	child_init  /* per-child init function */
};

/**
 * init module function
 */
static int mod_init(void)
{
	load_tm_f load_tm;
//	load_im_f load_im;
	int  i;

	DBG("XJAB:mod_init: initializing ...\n");

	/* import mysql functions */
	if (bind_dbmod())
	{
		DBG("XJAB:mod_init: error - database module not found\n");
		return -1;
	}
	db_con = (db_con_t**)shm_malloc(nrw*sizeof(db_con_t*));
	if (db_con == NULL)
	{
		DBG("XJAB:mod_init: Error while allocating db_con's\n");
		return -1;
	}

	/* import the TM auto-loading function */
	if ( !(load_tm=(load_tm_f)find_export("load_tm", NO_SCRIPT))) {
		LOG(L_ERR, "ERROR: xjab:mod_init: can't import load_tm\n");
		return -1;
	}
	/* let the auto-loading function load all TM stuff */
	if (load_tm( &tmb )==-1)
		return -1;

	pipes = (int**)pkg_malloc(nrw*sizeof(int*));
	if (pipes == NULL)
	{
		DBG("XJAB:mod_init: Error while allocating pipes\n");
		return -1;
	}
	
	for(i=0; i<nrw; i++)
	{
		pipes[i] = (int*)pkg_malloc(2*sizeof(int));
		if (!pipes[i])
		{
			DBG("XJAB:mod_init: Error while allocating pipes\n");
			return -1;
		}
	}
	
	for(i=0; i<nrw; i++)
	{
		db_con[i] = db_init(db_url);
		if (!db_con[i])
		{
			DBG("XJAB:mod_init: Error while connecting database\n");
			return -1;
		}
		else
		{
			db_use_table(db_con[i], db_table);
			DBG("XJAB:mod_init: Database connection opened successfuly\n");
		}
	}

	
	/** creating the pipees */
	
	for(i=0;i<nrw;i++)
	{
		/* create the pipe*/
		if (pipe(pipes[i])==-1) {
			DBG("XJAB:mod_init: error - cannot create pipe!\n");
			return -1;
		}
		DBG("XJAB:mod_init: pipe[%d] = <%d>-<%d>\n", i, pipes[i][0],
			pipes[i][1]);
	}
	
	if((jwl = xj_wlist_init(pipes,nrw,max_jobs,cache_time,sleep_time,
				delay_time)) == NULL)
	{
		DBG("XJAB:mod_init: error initializing workers list\n");
		return -1;
	}
	
	if(xj_wlist_init_contact(jwl, contact) < 0)
	{
		DBG("XJAB:mod_init: error initializing workers list properties\n");
		return -1;
	}

	if(xj_wlist_set_aliases(jwl, jaliases, jdomain) < 0)
	{
		DBG("XJAB:mod_init: error setting aliases\n");
		return -1;
	}
	
	DBG("XJAB:mod_init: initialized ...\n");	
	return 0;
}

/*
 * Initialize childs
 */
static int child_init(int rank)
{
	int i, j, mpid, cpid;
	
	DBG("XJAB:child_init: initializing child <%d>\n", rank);
	if(rank == 0)
	{
		if((mpid=fork())<0 )
		{
			DBG("XJAB:child_init: error - cannot launch worker's manager\n");
				return -1;
		}
		if(mpid == 0)
		{
			/** launching the workers */
			for(i=0;i<nrw;i++)
			{
				if ( (cpid=fork())<0 )
				{
					DBG("XJAB:child_init: error - cannot launch worker\n");
					return -1;
				}
				if (cpid == 0)
				{
					for(j=0;j<nrw;j++)
						if(j!=i) close(pipes[j][0]);
					close(pipes[i][1]);
					if(xj_wlist_set_pid(jwl, getpid(), i) < 0)
					{
						DBG("XJAB:child_init: error setting worker's pid\n");
						return -1;
					}
					xj_worker_process(jwl,jaddress,jport,i,db_con[i]);
					exit(0);
				}
			}

			mpid = getpid();
			while(1)
			{
				sleep(check_time);
				xjab_check_workers(mpid);
			}
			exit(0);
		}
	}
	
	//if(pipes)
	//{
	//	for(i=0;i<nrw;i++)
	//		close(pipes[i][0]);
	//}
	return 0;
}

/**
 * send the SIP MESSAGE through Jabber
 */
static int xj_send_message(struct sip_msg *msg, char* foo1, char * foo2)
{
	DBG("XJAB: processing SIP MESSAGE\n");
	return xjab_manage_sipmsg(msg, XJ_SEND_MESSAGE);
}

/**
 * join a Jabber conference
 */
static int xj_join_jconf(struct sip_msg *msg, char* foo1, char * foo2)
{
	DBG("XJAB: join a Jabber conference\n");
	return xjab_manage_sipmsg(msg, XJ_JOIN_JCONF);
}

/**
 * exit from Jabber conference
 */
static int xj_exit_jconf(struct sip_msg *msg, char* foo1, char * foo2)
{
	DBG("XJAB: exit from a Jabber conference\n");
	return xjab_manage_sipmsg(msg, XJ_EXIT_JCONF);
}

/**
 * go online in Jabber network
 */
static int xj_go_online(struct sip_msg *msg, char* foo1, char * foo2)
{
	DBG("XJAB: go online in Jabber network\n");
	return xjab_manage_sipmsg(msg, XJ_GO_ONLINE);
}

/**
 * go offline in Jabber network
 */
static int xj_go_offline(struct sip_msg *msg, char* foo1, char * foo2)
{
	DBG("XJAB: go offline in Jabber network\n");
	return xjab_manage_sipmsg(msg, XJ_GO_OFFLINE);
}

/**
 * manage SIP message
 */
int xjab_manage_sipmsg(struct sip_msg *msg, int type)
{
	str body, dst;
	xj_sipmsg jsmsg;
	struct to_body to, *from;
	struct sip_uri _uri;
	int pipe, fl;
	char   *buf=0;
	t_xj_jkey jkey, *p;

	// extract message body - after that whole SIP MESSAGE is parsed
	if (type==XJ_SEND_MESSAGE)
	{
		/* get th content-type and content-length headers */
		if (parse_headers( msg, HDR_CONTENTLENGTH|HDR_CONTENTTYPE, 0)==-1
		|| !msg->content_type || !msg->content_length) 
		{
			LOG(L_ERR,"XJAB:xjab_manage_sipmsg: ERROR fetching content-lenght"
				" and content_type failed! Parse error or headers missing!\n");
                goto error;
        }

		/* check the content-type value */
		if ( (int)msg->content_type->parsed!=CONTENT_TYPE_TEXT_PLAIN
		&& (int)msg->content_type->parsed!=CONTENT_TYPE_MESSAGE_CPIM ) 
		{
			LOG(L_ERR,"XJAB:xjab_manage_sipmsg: ERROR invalid content-type for"
				" a message request! type found=%d\n",
				(int)msg->content_type->parsed);
			goto error;
		}

		/* get the message's body */
		body.s = get_body( msg );
		if (body.s==0) 
		{
			LOG(L_ERR,"XJAB:xjab_manage_sipmsg: ERROR cannot extract body from"
				" msg\n");
			goto error;
		}
		body.len = (int)msg->content_length->parsed;
	}
	
	// check for TO and FROM headers 
	if(parse_headers( msg, HDR_TO|HDR_FROM, 0)==-1 || !msg->to || !msg->from)
	{
		LOG(L_ERR,"XJAB:xjab_manage_sipmsg: cannot find TO or FROM HEADERS!\n");
		goto error;
	}
	
	/* parsing from header */
	if ( parse_from_header( msg )==-1 ) 
	{
		DBG("ERROR:xjab_manage_sipmsg: cannot get FROM header\n");
		goto error;
	}
	from = (struct to_body*)msg->from->parsed;

	jkey.hash = xj_get_hash(&from->uri, NULL);
	jkey.id = &from->uri;
	// get the communication pipe with the worker
	switch(type)
	{
		case XJ_SEND_MESSAGE:
		case XJ_JOIN_JCONF:
		case XJ_GO_ONLINE:
			if((pipe = xj_wlist_get(jwl, &jkey, &p)) < 0)
			{
				DBG("XJAB:xjab_manage_sipmsg: cannot find pipe of the worker!\n");
				goto error;
			}
		break;
		case XJ_EXIT_JCONF:
		case XJ_GO_OFFLINE:
			if((pipe = xj_wlist_check(jwl, &jkey, &p)) < 0)
/**
 *
 */
			{
				DBG("XJAB:xjab_manage_sipmsg: no open Jabber session for"
						" <%.*s>!\n", from->uri.len, from->uri.s);
				goto error;
			}
		break;
		default:
			DBG("XJAB:xjab_manage_sipmsg: ERROR:strange SIP msg type!\n");
			goto error;
	}
	
	// determination of destination
	dst.len = 0;
	if( msg->new_uri.len > 0 )
	{
		DBG("XJAB:xjab_manage_sipmsg: using NEW URI for destination\n");
		dst.s = msg->new_uri.s;
		dst.len = msg->new_uri.len;
	} else if ( msg->first_line.u.request.uri.len > 0 )
	{
		DBG("XJAB:xjab_manage_sipmsg: parsing URI from first line\n");
		if(parse_uri(msg->first_line.u.request.uri.s,
					msg->first_line.u.request.uri.len, &_uri) < 0)
		{
			DBG("XJAB:xjab_manage_sipmsg: ERROR parsing URI from first line\n");
			goto error;
		}
		if(_uri.user.len > 0)
		{
			DBG("XJAB:xjab_manage_sipmsg: using URI for destination\n");
			dst.s = msg->first_line.u.request.uri.s;
			dst.len = msg->first_line.u.request.uri.len;
		}
	}
	if(dst.len == 0 && msg->to != NULL)
	{
		if(msg->to->parsed)
		{
			DBG("XJAB:xjab_manage_sipmsg: TO already parsed\n");
			dst.s = ((struct to_body*)msg->to->parsed)->uri.s;
			dst.len = ((struct to_body*)msg->to->parsed)->uri.len;
		}
		else
		{
			DBG("XJAB:xjab_manage_sipmsg: TO NOT parsed -> parsing ...\n");
			memset( &to , 0, sizeof(to) );
			parse_to(msg->to->body.s, msg->to->body.s + msg->to->body.len + 1,
				&to);
			if(to.uri.len > 0) // to.error == PARSE_OK)
			{
				DBG("XJAB:xjab_manage_sipmsg: TO parsed OK <%.*s>.\n",
					to.uri.len, to.uri.s);
				dst.s = to.uri.s;
				dst.len = to.uri.len;
			}
			else
			{
				DBG("XJAB:xjab_manage_sipmsg: error parsing TO header.\n");
				goto error;
			}
		}
	}
	if(dst.len == 0)
	{
		DBG("XJAB:xjab_manage_sipmsg: destination not found in SIP message\n");
		goto error;
	}
	
	/** skip 'sip:' in destination address */
	if(dst.s[0]=='s' && dst.s[1]=='i' && dst.s[2]=='p')
	{
		dst.s += 3;
		dst.len -= 3;
		fl = 1;
		while(*dst.s == ' ' || *dst.s == '\t' || *dst.s == ':')
		{
			dst.s++;
			dst.len--;
			fl = 0;
		}
		if(fl)
		{
			dst.s -= 3;
			dst.len += 3;
		}
		
		DBG("XJAB:xjab_manage_sipmsg: DESTINATION corrected [%.*s].\n", 
				dst.len, dst.s);
	}
	
	//putting the SIP message parts in share memory to be accessible by workers
    jsmsg = (xj_sipmsg)shm_malloc(sizeof(t_xj_sipmsg));
	memset(jsmsg, 0, sizeof(t_xj_sipmsg));
    if(jsmsg == NULL)
    	return -1;
	
	switch(type)
	{
		case XJ_SEND_MESSAGE:
			jsmsg->msg.len = body.len;
			if((jsmsg->msg.s = (char*)shm_malloc(jsmsg->msg.len+1)) == NULL)
			{
				shm_free(jsmsg);
				goto error;
			}
			strncpy(jsmsg->msg.s, body.s, jsmsg->msg.len);
		break;
		case XJ_JOIN_JCONF:
		case XJ_EXIT_JCONF:
		case XJ_GO_ONLINE:
		case XJ_GO_OFFLINE:
			jsmsg->msg.len = 0;
			jsmsg->msg.s = NULL;
		break;
		default:
			DBG("XJAB:xjab_manage_sipmsg: this SHOULD NOT appear\n");
			shm_free(jsmsg);
			goto error;
	}
	jsmsg->to.len = dst.len;
	if((jsmsg->to.s = (char*)shm_malloc(jsmsg->to.len+1)) == NULL)
	{
		if(type == XJ_SEND_MESSAGE)
			shm_free(jsmsg->msg.s);
		shm_free(jsmsg);
		goto error;
	}
	strncpy(jsmsg->to.s, dst.s, jsmsg->to.len);

	jsmsg->jkey = p;
	jsmsg->type = type;
	//jsmsg->jkey->hash = jkey.hash;

	DBG("XJAB:xjab_manage_sipmsg:%d: sending <%p> to worker through <%d>\n",
			getpid(), jsmsg, pipe);
	// sending the SHM pointer of SIP message to the worker
	fl = write(pipe, &jsmsg, sizeof(jsmsg));
	if(fl != sizeof(jsmsg))
	{
		DBG("XJAB:xjab_manage_sipmsg: error when writting to worker pipe!\n");
		if(type == XJ_SEND_MESSAGE)
			shm_free(jsmsg->msg.s);
		shm_free(jsmsg->to.s);
		shm_free(jsmsg);
		goto error;
	}
	
	if (buf) 
		pkg_free(buf);	
	return 1;
error:
	if (buf) 
		pkg_free(buf);
	return -1;
}

/**
 * destroy function of module
 */
void destroy(void)
{
	int i;
	DBG("XJAB: Unloading module ...\n");
	if(pipes)
	{ // close the pipes
		for(i = 0; i < nrw; i++)
		{
			if(pipes[i])
			{
				close(pipes[i][0]);
				close(pipes[i][1]);
			}
			pkg_free(pipes[i]);
		}
		pkg_free(pipes);
	}
	// cleaning MySQL connections
	if(db_con != NULL)
	{
		for(i = 0; i<nrw; i++)
			db_close(db_con[i]);
		shm_free(db_con);
	}
			
	xj_wlist_free(jwl);
	DBG("XJAB: Unloaded\n");
}

/**
 * register a watcher function for a Jabber user' presence
 */
void xj_register_watcher(str *from, str *to, void *cbf, void *pp)
{
	xj_sipmsg jsmsg = NULL;
	t_xj_jkey jkey, *jp;
	int pipe, fl;

	if(!to || !from || !cbf)
		return;

	DBG("XJAB:xj_register_watcher: from=[%.*s] to=[%.*s]\n", from->len,
			from->s, to->len, to->s);
	jkey.hash = xj_get_hash(from, NULL);
	jkey.id = from;

	if((pipe = xj_wlist_get(jwl, &jkey, &jp)) < 0)
	{
		DBG("XJAB:xj_register_watcher: cannot find pipe of the worker!\n");
		goto error;
	}
	
	//putting the SIP message parts in share memory to be accessible by workers
    jsmsg = (xj_sipmsg)shm_malloc(sizeof(t_xj_sipmsg));
	memset(jsmsg, 0, sizeof(t_xj_sipmsg));
    if(jsmsg == NULL)
    	goto error;
	
	jsmsg->msg.len = 0;
	jsmsg->msg.s = NULL;
	
	jsmsg->to.len = to->len;
	if((jsmsg->to.s = (char*)shm_malloc(jsmsg->to.len+1)) == NULL)
	{
		if(jsmsg->msg.s)
			shm_free(jsmsg->msg.s);
		shm_free(jsmsg);
		goto error;
	}
	strncpy(jsmsg->to.s, to->s, jsmsg->to.len);

	jsmsg->jkey = jp;
	jsmsg->type = XJ_REG_WATCHER;
	//jsmsg->jkey->hash = jkey.hash;
	
	jsmsg->cbf = (pa_callback_f)cbf;
	jsmsg->p = pp;

	DBG("XJAB:xj_register_watcher:%d: sending <%p> to worker through <%d>\n",
			getpid(), jsmsg, pipe);
	// sending the SHM pointer of SIP message to the worker
	fl = write(pipe, &jsmsg, sizeof(jsmsg));
	if(fl != sizeof(jsmsg))
	{
		DBG("XJAB:xj_register_watcher: error when writting to worker pipe!\n");
		if(jsmsg->msg.s)
			shm_free(jsmsg->msg.s);
		shm_free(jsmsg->to.s);
		shm_free(jsmsg);
		goto error;
	}
	
error:
	return;
}

/**
 * unregister a watcher for a Jabber user' presence
 */
void xj_unregister_watcher(str *from, str *to, void *cbf, void *pp)
{
	if(!to || !from)
		return;
}

/**
 *
 */
void xjab_check_workers(int mpid)
{
	int i, n, stat;
	//DBG("XJAB:%d:xjab_check_workers: time=%d\n", mpid, get_ticks());
	if(!jwl || jwl->len <= 0)
		return;
	for(i=0; i < jwl->len; i++)
	{
		if(jwl->workers[i].pid <= 0)
			continue;
				stat = 0;
		n = waitpid(jwl->workers[i].pid, &stat, WNOHANG);
		if(n == 0)
			continue;
		
		LOG(L_ERR,"XJAB:xjab_check_workers: worker[%d][pid=%d] has exited"
			" - status=%d err=%d errno=%d\n", i, jwl->workers[i].pid, 
			stat, n, errno);
		if(n==jwl->workers[i].pid)
		{
			DBG("XJAB:%d:xjab_check_workers: create a new worker\n", mpid);
			xj_wlist_clean_jobs(jwl, i, 1);
			xj_wlist_set_pid(jwl, -1, i);
			if ( (stat=fork())<0 )
			{
				DBG("XJAB:xjab_check_workers: error - cannot launch worker\n");
				return;
			}
			if (stat == 0)
			{
				if(xj_wlist_set_pid(jwl, getpid(), i) < 0)
				{
					DBG("XJAB:xjab_check_workers: error setting worker's pid\n");
					return;
				}
				xj_worker_process(jwl,jaddress,jport,i,db_con[i]);
				exit(0);
			}
		}
		else
		{
			LOG(L_ERR, "XJAB:xjab_check_workers: error - worker[%d][pid=%d] lost"
				" forever\n", i, jwl->workers[i].pid);
			xj_wlist_set_pid(jwl, -1, i);
		}
	}			
}

