/*
 * $Id: jabber.c,v 1.16 2002/10/28 11:47:12 dcm Exp $
 *
 * JABBER module
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
#include <sys/ipc.h>
#include <unistd.h>
#include <fcntl.h>

#include "../../sr_module.h"
#include "../../error.h"
#include "../../ut.h"
#include "../../mem/shm_mem.h"
#include "../../mem/mem.h"
#include "../../globals.h"
#include "../../parser/parse_uri.h"

#include "../im/im_load.h"
#include "../tm/tm_load.h"

#include "xjab_worker.h"
#include "xjab_util.h"
#include "../../db/db.h"

/** TM bind */
struct tm_binds tmb;
/** IM binds */
struct im_binds imb;

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

int **pipes = NULL;

static int mod_init(void);
static int child_init(int rank);
static int jab_send_message(struct sip_msg*, char*, char* );

void destroy(void);

struct module_exports exports= {
	"jabber",
	(char*[]){
		"jab_send_message"
	},
	(cmd_function[]){
		jab_send_message
	},
	(int[]){
		0
	},
	(fixup_function[]){
		0
	},
	1,

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
		"sleep_time"
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
		&sleep_time
	},
	11,      /* Number of module paramers */
	
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
	load_im_f load_im;
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

	/** import the IM auto-loading function */
	if ( !(load_im=(load_im_f)find_export("load_im", 1))) 
	{
		LOG(L_ERR, "ERROR: sms: global_init: cannot import load_im\n");
		return -1;
	}
	/* let the auto-loading function load all IM stuff */
	if (load_im( &imb )==-1)
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
	
	if((jwl = xj_wlist_init(pipes, nrw, max_jobs)) == NULL)
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
	int i;
	int *pids = NULL;
	
	DBG("XJAB:child_init: initializing child <%d>\n", rank);
	if(rank == 0)
	{
		pids = (int*)pkg_malloc(nrw*sizeof(int));
		if (pids == NULL)
		{
			DBG("XJAB:child_init: error while allocating pid's\n");
			return -1;
		}
		/** launching the workers */
		for(i=0;i<nrw;i++)
		{
			if ( (pids[i]=fork())<0 )
			{
				DBG("XJAB:child_init: error - cannot launch worker\n");
				return -1;
			}
			if (pids[i] == 0)
			{
				close(pipes[i][1]);
				xj_worker_process(jwl, jaddress, jport, pipes[i][0], max_jobs,
					cache_time,	sleep_time, delay_time, db_con[i]);
				exit(0);
			}
		}
	
		if(xj_wlist_set_pids(jwl, pids, nrw) < 0)
		{
			DBG("XJAB:child_init: error setting pid's\n");
			return -1;
		}
		if(pids)
			pkg_free(pids);
	}
	
	if(pipes)
	{
		for(i=0;i<nrw;i++)
			close(pipes[i][0]);
	}
	return 0;
}

/**
 * send the SIP message through Jabber
 */
static int jab_send_message(struct sip_msg *msg, char* foo1, char * foo2)
{
	str body, dst, /*host, user,*/ *p;
	xj_sipmsg jsmsg;
	struct to_body to, from;
	struct sip_uri _uri;
	int pipe, fl;
	struct to_param *foo,*bar;
	char   *cp, *buf=0;

	// extract message body - after that whole SIP MESSAGE is parsed
	if ( imb.im_extract_body(msg,&body)==-1 )
	{
		LOG(L_ERR,"ERROR:XJAB:xjab_send_message: cannot extract body"
				" from sip msg!\n");
		goto error;
	}

	
	// check for FROM header
	if(!msg->from)
	{
		LOG(L_ERR,"XJAB: xjab_send_message: cannot find FROM HEADER!\n");
		goto error;
	}
	
	/* parsing from header */
	memset(&from,0,sizeof(from));
	cp = translate_pointer(msg->orig,msg->buf,msg->from->body.s);
	buf = (char*)pkg_malloc(msg->from->body.len+1);
	if (!buf) 
	{
		DBG("XJAB: xjab_send_message: error no free pkg memory\n");
		goto error;
	}
	memcpy(buf,cp,msg->from->body.len+1);
	parse_to(buf,buf+msg->from->body.len+1,&from);
	if (from.error!=PARSE_OK ) 
	{
		DBG("XJAB: xjab_send_message: error cannot parse from header\n");
		goto error;
	}
	/* we are not intrested in from param-> le's free them now*/
	for(foo=from.param_lst ; foo ; foo=bar)
	{
		bar = foo->next;
		pkg_free(foo);
	}
	
	// get the communication pipe with the worker
	if((pipe = xj_wlist_get(jwl, &from.uri, &p)) < 0)
	{
		DBG("XJAB: xjab_send_message: cannot find pipe of the worker!\n");
		goto error;
	}
	
	// determination of destination
	dst.len = 0;
	if( msg->new_uri.len > 0 )
	{
		DBG("XJAB: xjab_send_message: using NEW URI for destination\n");
		dst.s = msg->new_uri.s;
		dst.len = msg->new_uri.len;
	} else if ( msg->first_line.u.request.uri.len > 0 )
	{
		DBG("XJAB: xjab_send_message: parsing URI from first line\n");
		if(parse_uri(msg->first_line.u.request.uri.s,
					msg->first_line.u.request.uri.len, &_uri) < 0)
		{
			DBG("XJAB: xjab_send_message: ERROR parsing URI from first line\n");
			goto error;
		}
		if(_uri.user.len > 0)
		{
			DBG("XJAB: xjab_send_message: using URI for destination\n");
			dst.s = msg->first_line.u.request.uri.s;
			dst.len = msg->first_line.u.request.uri.len;
		}
		free_uri(&_uri);
	}
	if(dst.len == 0 && msg->to != NULL)
	{
		memset( &to , 0, sizeof(to) );
		parse_to(msg->to->body.s, msg->to->body.s + msg->to->body.len + 1,
				&to);
		if(to.uri.len > 0) // to.error == PARSE_OK)
		{
			DBG("XJAB: xjab_send_message: TO parsed OK <%.*s>.\n",
				to.uri.len, to.uri.s);
			dst.s = to.uri.s;
			dst.len = to.uri.len;
		}
		else
		{
			DBG("XJAB: xjab_send_message: TO NOT parsed\n");
			goto error;
		}
	}
	if(dst.len == 0)
	{
		DBG("XJAB: xjab_send_message: destination not found in SIP message\n");
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
		
		DBG("XJAB: xjab_send_message: DESTINATION corrected <%.*s>.\n", 
				dst.len, dst.s);
	}
	
	//putting the SIP message parts in share memory to be accessible by workers
    jsmsg = (xj_sipmsg)shm_malloc(sizeof(t_xj_sipmsg));
    if(jsmsg == NULL)
    	return -1;
	jsmsg->to.len = dst.len;
	jsmsg->to.s = (char*)shm_malloc(jsmsg->to.len+1);
	if(jsmsg->to.s == NULL)
	{
		shm_free(jsmsg);
		goto error;
	}
	strncpy(jsmsg->to.s, dst.s, jsmsg->to.len);
	
	jsmsg->msg.len = body.len;
	jsmsg->msg.s = (char*)shm_malloc(jsmsg->msg.len+1);
	if(jsmsg->msg.s == NULL)
	{
		shm_free(jsmsg->to.s);
		shm_free(jsmsg);
		goto error;
	}
	strncpy(jsmsg->msg.s, body.s, jsmsg->msg.len);
	
	jsmsg->from = p;
	
	DBG("XJAB: xjab_send_message:%d: sending <%p> to worker through <%d>\n",
			getpid(), jsmsg, pipe);
	// sending the SHM pointer of SIP message to the worker
	if(write(pipe, &jsmsg, sizeof(jsmsg)) != sizeof(jsmsg))
	{
		DBG("XJAB: xjab_send_message: error when writting to worker pipe!\n");
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
	{
		for(i = 0; i < nrw; i++)
			pkg_free(pipes[i]);
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

