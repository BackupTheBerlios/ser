/*
 * $Id: jc_pool.c,v 1.9 2002/09/19 12:23:53 jku Exp $
 *
 * JABBER module - Jabber connections pool
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


#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>

#include "../../dprint.h"
#include "../../timer.h"
#include "../../mem/mem.h"
#include "../../mem/shm_mem.h"

#include "jc_pool.h"
#include "sip2jabber.h"
#include "../im/im_funcs.h"
#include "xml_jab.h"
#include "mdefines.h"


/**
 * function used to compare two elements in B-Tree
 */
int k_cmp(void *a, void *b)
{
	int n;
	if(a == NULL)
	    return -1;
	if(a == NULL)
	    return 1;
	// DBG("JABBER: k_kmp: comparing <%.*s> / <%.*s>\n", ((str *)a)->len,
	// 		((str *)a)->s, ((str *)b)->len, ((str *)b)->s);
	if(((str *)a)->len != ((str *)b)->len)
		return -1;
	n = strncmp(((str *)a)->s, ((str *)b)->s, ((str *)a)->len);
	if(n<0)
		return -1;
	if(n>0)
		return 1;
	return 0;
}

/**
 * free the information from a B-Tree node
 */
void free_str_p(void *p)
{
	if(p == NULL)
		return;
	if(((str*)p)->s != NULL)
		_M_SHM_FREE(((str*)p)->s);
	_M_SHM_FREE(p);
}

/**
 * free a pointer to a t_jab_sipmsg structure
 * > element where points 'from' MUST be eliberated separated
 */
void jab_sipmsg_free(jab_sipmsg jsmsg)
{
	if(jsmsg == NULL)
		return;
	if(jsmsg->to.s != NULL)
		_M_SHM_FREE(jsmsg->to.s);
//	if(jsmsg->from.s != NULL)
//		_M_SHM_FREE(jsmsg->from.s);
	if(jsmsg->msg.s != NULL)
		_M_SHM_FREE(jsmsg->msg.s);
	_M_SHM_FREE(jsmsg);
}

/**
 * init a workers list
 * - pipes : communication pipes
 * - size : size of list - number of workers
 * - max : maximum number of jobs per worker
 * #return : pointer to workers list or NULL on error
 */
jab_wlist jab_wlist_init(int **pipes, int size, int max)
{
	int i;
	jab_wlist jwl = NULL;
	
	if(pipes == NULL || size <= 0 || max <= 0)
		return NULL;
	
	DBG("JABBER: JC_WLIST_INIT: --- === ---\n");
	
	jwl = (jab_wlist)_M_SHM_MALLOC(sizeof(t_jab_wlist));
	if(jwl == NULL)
		return NULL;
	jwl->len = size;
	jwl->maxj = max;
	jwl->contact_h = NULL;
	if((jwl->sems = create_semaphores(size)) == NULL)
	{
		_M_SHM_FREE(jwl);
		return NULL;
	}
	jwl->workers = (jab_worker)_M_SHM_MALLOC(size*sizeof(t_jab_worker));
	if(jwl->workers == NULL)
	{
		_M_SHM_FREE(jwl);
		return NULL;
	}
	
	for(i = 0; i < size; i++)
	{
		jwl->workers[i].nr = 0;
		jwl->workers[i].pipe = pipes[i][1];
		if((jwl->workers[i].sip_ids = newtree234(k_cmp)) == NULL)
		{
			_M_SHM_FREE(jwl);
			return NULL;
		}
	}	
	
	return jwl;
}

/**
 * init contact address for SIP messages that will be sent by workers
 * - jwl - pointer to workers list
 * - ch - string representation of the contact, e.g. 'sip:100.100.100.100:5060'
 * #return : 0 on success or <0 on error
 * info: still has 0 at the end of string
 */
int jab_wlist_init_contact(jab_wlist jwl, char *ch)
{
	int f = 0; // correction flag: 1 -> must be converted to <sip: ... > 
	if(ch == NULL)
		return -1;
	if((jwl->contact_h = (str*)_M_SHM_MALLOC(sizeof(str))) == NULL)
		return -1;
	jwl->contact_h->len = strlen(ch);

	if(jwl->contact_h->len > 2 && strstr(ch, "sip:") == NULL)
	{
		// contact correction
		jwl->contact_h->len += 6;
		f = 1;
	}

	if((jwl->contact_h->s=(char*)_M_SHM_MALLOC(jwl->contact_h->len+1))==NULL)
	{
		_M_SHM_FREE(jwl->contact_h);
		return -2;
	}
	
	if(f)
	{
		strncpy(jwl->contact_h->s, "<sip:", 5);
		strcpy(jwl->contact_h->s+5, ch);
		jwl->contact_h->s[jwl->contact_h->len-1] = '>';
		jwl->contact_h->s[jwl->contact_h->len] = 0;
	}
	else
		strcpy(jwl->contact_h->s, ch);

	return 0;
}


/**
 * set the p.id's of the workers
 * - jwl : pointer to the workers list
 * - pids : p.id's array
 * - size : number of pids
 * #return : 0 on success or <0 on error
 */
int jab_wlist_set_pids(jab_wlist jwl, int *pids, int size)
{
	int i;
	
	if(jwl == NULL || pids == NULL || size <= 0)
		return -1;
	for(i = 0; i < size; i++)
		jwl->workers[i].pid = pids[i];
	return 0;
}

/**
 * free jab_wlist
 * - jwl : pointer to the workers list
 */
void jab_wlist_free(jab_wlist jwl)
{
	int i;
	DBG("JABBER: jab_wlist_free : freeing 'jab_wlist' memory ...\n");
	if(jwl == NULL)
		return;
	
	if(jwl->contact_h != NULL && jwl->contact_h->s != NULL)
		_M_SHM_FREE(jwl->contact_h->s);
	if(jwl->contact_h != NULL)
		_M_SHM_FREE(jwl->contact_h);
		
	if(jwl->workers != NULL)
	{
		for(i=0; i<jwl->len; i++)
			free2tree234(jwl->workers[i].sip_ids, free_str_p);
		_M_SHM_FREE(jwl->workers);
	}
	
	//rm_sem(jwl->semid);
	if(jwl->sems != NULL)
		destroy_semaphores(jwl->sems);
	
	_M_SHM_FREE(jwl);
}

/**
 * return communication pipe with the worker that will process the message for
 * 		the id 'sid', or -1 if error
 * - jwl : pointer to the workers list
 * - sid : id of the entity (connection to Jabber - usually SHOULD be FROM 
 *   header of the incomming SIP message)
 * - p : will point to the SHM location of the 'sid' in jwl
 */
int jab_wlist_get(jab_wlist jwl, str *sid, str **p)
{
	int i = 0, pos = -1, min = 100000;
	str *msid = NULL;
	
	if(jwl == NULL)
		return -1;
	DBG("JABBER: JC_WLIST_GET: --- === ---\n");
	//mutex_lock(jwl->semid);
	
	*p = NULL;
	while(i < jwl->len)
	{
		s_lock_at(jwl->sems, i);
		if((*p = find234(jwl->workers[i].sip_ids, (void*)sid, NULL)) != NULL)
		{
			//mutex_unlock(jwl->semid);
			if(pos >= 0)
				s_unlock_at(jwl->sems, pos);
			s_unlock_at(jwl->sems, i);
			DBG("JABBER: JC_WLIST_GET: entry already exists for <%.*s> in the" 
				" pool of <%d> [%d]\n",sid->len, sid->s,jwl->workers[i].pid,i);
			return jwl->workers[i].pipe;
		}
		if(min > jwl->workers[i].nr)
		{
			if(pos >= 0)
				s_unlock_at(jwl->sems, pos);
			pos = i;
			min = jwl->workers[i].nr;
		}
		else
			s_unlock_at(jwl->sems, i);
		i++;
	}
	if(pos >= 0 && jwl->workers[pos].nr < jwl->maxj)
	{
		jwl->workers[pos].nr++;
		
		msid = (str*)_M_SHM_MALLOC(sizeof(str));
		if((msid != NULL) && 
			(*p = add234(jwl->workers[pos].sip_ids, msid)) != NULL)
		{
			msid->s = (char*)_M_SHM_MALLOC(sid->len);
			msid->len = sid->len;
			memcpy(msid->s, sid->s, sid->len);
			s_unlock_at(jwl->sems, pos);
			//mutex_unlock(jwl->semid);
			DBG("JABBER: JC_WLIST_GET: new entry for <%.*s> in the pool of"
				" <%d> - [%d]\n", sid->len, sid->s,
				jwl->workers[pos].pid, pos);
			return jwl->workers[pos].pipe;
		}
	}
	
	if(pos >= 0)
		s_unlock_at(jwl->sems, pos);
	//mutex_unlock(jwl->semid);
	
	return -1;
}

/**
 * delete an entity from working list of a worker
 * - jwl : pointer to the workers list
 * - sid : id of the entity (connection to Jabber - usually SHOULD be FROM
 *   header of the incomming SIP message
 * - _pid : process id of the worker
 */
void jab_wlist_del(jab_wlist jwl, str *sid, int _pid)
{
	int i;
	void *p;
	if(jwl == NULL || sid == NULL)
		return;
	for(i=0; i < jwl->len; i++)
		if(jwl->workers[i].pid == _pid)
			break;
	if(i >= jwl->len)
	{
		DBG("JABBER: jab_wlist_del:%d: key <%.*s> not found in [%d]...\n", 
			_pid, sid->len, sid->s, i);		
		return;
	}
	DBG("JABBER: jab_wlist_del:%d: trying to delete entry for <%.*s>...\n", 
		_pid, sid->len, sid->s);
	
	s_lock_at(jwl->sems, i);
	p = del234(jwl->workers[i].sip_ids, (void*)sid);	
	
	if(p != NULL)
	{
		jwl->workers[i].nr--;
		
		DBG("JABBER: jab_wlist_del:%d: sip id <%.*s> deleted\n", _pid, 
			sid->len, sid->s);
		free_str_p(p);
	}
		
	s_unlock_at(jwl->sems, i);
}

/**
 * send a SIP MESSAGE message
 * - to : destination
 * - from : origin
 * - contact : contact header
 * - msg : body of the message
 * #return : 0 on success or <0 on error
 */
int jab_send_sip_msg(str *to, str *from, str *contact, str *msg)
{
	char buf[512];
	str tfrom;
	// from correction
	strcpy(buf, "<sip:");
	strncat(buf, from->s, from->len);
	tfrom.len = from->len;
	if(strstr(buf+5, "sip:") == NULL)
	{
		tfrom.len += 5;
		buf[tfrom.len++] = '>';
		tfrom.s = buf;
	}
	else
		tfrom.s = buf+5;
	if(contact != NULL && contact->len > 2)
	    return im_send_message(to, to, &tfrom, contact, msg);
	else
	    return im_send_message(to, to, &tfrom, &tfrom, msg);
}

/**
 * send a SIP MESSAGE message
 * - to : destination
 * - from : origin
 * - contact : contact header
 * - msg : body of the message, string terminated by zero
 * #return : 0 on success or <0 on error
 */
int jab_send_sip_msgz(str *to, str *from, str *contact, char *msg)
{
	str tstr;
	int n;
	tstr.s = msg;
	tstr.len = strlen(msg);
	if((n = jab_send_sip_msg(to, from, contact, &tstr)) < 0)
		DBG("JABBER: jab_send_sip_msgz: ERROR SIP MESSAGE wasn't sent to"
			" [%.*s]...\n", tstr.len, tstr.s);
	else
		DBG("JABBER: jab_send_sip_msgz: SIP MESSAGE was sent to [%.*s]...\n", 
			tstr.len, tstr.s);
	return n;
}

/**
 * worker implementation
 * - jwl : pointer to the workers list
 * - jaddress : address of the jabber server
 * - jport : port of the jabber server
 * - pipe : communication pipe with SER
 * - size : maximun number of jobs - open connections to Jabber server
 * - ctime : cache time for a connection to Jabber
 * - wtime : wait time between cache checking
 * - dtime : delay time for first message
 * - db_con : connection to database
 * #return : 0 on success or <0 on error
 */
int worker_process(jab_wlist jwl, char* jaddress, int jport, int pipe,
		int size, int ctime, int wtime, int dtime, db_con_t* db_con)
{
	int ret, i, n, maxfd, error, cseq;
	jc_pool jcp;
	struct timeval tmv;
	fd_set set, mset;
	jab_sipmsg jsmsg;
	t_jab_jmsg tjmsg;
	jbconnection jbc;
	open_jc ojc;
	char buff[1024], tbuff[1024], recv_buff[4096];
	int ltime = 0;
	int _pid = getpid();
	str tstr;
	
	db_key_t keys[] = {"sip_id"};
	db_val_t vals[] = {{DB_STRING, 0, {.string_val = buff}}};
	db_key_t col[] = {"jab_id", "jab_passwd"};
	db_res_t* res = NULL;
	
	DBG("JABBER: WORKER_PROCESS:%d: started - pipe=<%d> : 1st message delay"
		" <%d>\n", _pid, pipe, dtime);
	
	if((jcp = jc_pool_init(size, 10)) == NULL)
	{
		DBG("JABBER: WORKER_PROCESS: cannot allocate the pool\n");
		return -1;		
	}
	
	maxfd = pipe;
	tmv.tv_sec = wtime;
	tmv.tv_usec = 0;
	cseq = 1;
	
	FD_ZERO(&set);
	FD_SET(pipe, &set);
	while(1)
	{
		mset = set;
			
		tmv.tv_sec = (jcp->jmqueue.size == 0)?wtime:1;
		DBG("JABBER: worker_process:%d: select waiting %dsec\n", _pid, 
						(int)tmv.tv_sec);
		tmv.tv_usec = 0;
		
		ret = select(maxfd+1, &mset, NULL, NULL, &tmv);
		/** check the queue AND conecction of head element is ready */
		while(jcp->jmqueue.size != 0 
			&& jcp->jmqueue.ojc[jcp->jmqueue.head]->ready < get_ticks())
		{
			/** send message from queue */
			DBG("JABBER: worker_process:%d: SENDING AS JABBER MESSAGE FROM "
				" LOCAL QUEUE ...\n", _pid);
			jb_send_msg(jcp->jmqueue.ojc[jcp->jmqueue.head]->jbc, 
						jcp->jmqueue.jsm[jcp->jmqueue.head]->to.s, 
						jcp->jmqueue.jsm[jcp->jmqueue.head]->to.len,
						jcp->jmqueue.jsm[jcp->jmqueue.head]->msg.s, 
						jcp->jmqueue.jsm[jcp->jmqueue.head]->msg.len);
			
			jab_sipmsg_free(jcp->jmqueue.jsm[jcp->jmqueue.head]);
			
			/** delete message from queue */
			jc_pool_del_jmsg(jcp);
		}
		
		error = 0;
		if(ret > 0)
		{
			DBG("JABBER: worker_process:%d: something is coming\n", _pid);
			if(FD_ISSET(pipe, &mset))
			{ // new job from ser
				read(pipe, &jsmsg, sizeof(jsmsg));

				DBG("JABBER: worker_process <%d>: job <%p> from SER\n", 
								_pid, jsmsg);
				if( jsmsg == NULL || jsmsg->from == NULL)
					continue;
				
				strncpy(buff, jsmsg->from->s, jsmsg->from->len);
				buff[jsmsg->from->len] = 0;
				
				ojc = jc_pool_get(jcp, jsmsg->from);
				if(ojc == NULL)
				{ // NO OPEN CONNECTION FOR THIS SIP ID
					DBG("JABBER:worker_process:%d: new connection for <%s>.\n",
									_pid, buff);
					if(db_query(db_con, keys, vals, col, 1, 2, NULL, &res)==0)
					{

						if (RES_ROW_N(res) != 0)
						{
						
							jbc = jb_init_jbconnection(jaddress, jport);
    	        		
							if(!jb_connect_to_server(jbc))
                            {
								DBG("JABBER: auth to jabber as: [%s] / [%s]\n",
									(char*)(ROW_VALUES(RES_ROWS(res))[0].val.string_val), 
									(char*)(ROW_VALUES(RES_ROWS(res))[1].val.string_val));
								if(jb_user_auth_to_server(jbc,
									(char*)(ROW_VALUES(RES_ROWS(res))[0].val.string_val),
									(char*)(ROW_VALUES(RES_ROWS(res))[1].val.string_val), "jbcl") == 0)
	        		            {
									ojc = open_jc_create(jsmsg->from, jbc, 
													ctime, dtime);
									if((ojc != NULL) 
										&& (jc_pool_add( jcp, ojc) == 0))
		                            {
										/** add socket descriptor to select */
										DBG("JABBER: worker_process <%d>: add"
											" connection on <%d> \n", _pid, 
											jbc->sock);
										if(jbc->sock > maxfd)
											maxfd = jbc->sock;
										FD_SET(jbc->sock, &set);
										
										jb_get_roster(jbc);
										jb_send_presence(jbc, NULL,
											"Online", "9");
										/** wait for a while - SER is tired */
										//sleep(3);
									}
									else
									{
										DBG("JABBER:worker_process:%d: Keeping"
											" connection to Jabber server"
											" failed! Not enough memory ...\n", 
											_pid);
										jb_disconnect(jbc);
										jb_free_jbconnection(jbc);
										if(ojc != NULL)
											open_jc_free(ojc);
										jab_send_sip_msgz(jsmsg->from, 
											&jsmsg->to,	jwl->contact_h,	
											"ERROR:Your message was	not"
											" sent. SIP-2-JABBER"
											" gateway is full.");
										error = 1;
									}	
								}
								else
								{
									DBG("JABBER:worker_process:%d: "
										" Authentication to the Jabber server"
										" failed ...\n", _pid);
									jb_disconnect(jbc);
									jb_free_jbconnection(jbc);
									jab_send_sip_msgz(jsmsg->from, &jsmsg->to,
										jwl->contact_h, "ERROR: Your message"
										" was not sent. Authentication to the"
										" Jabber server failed.");
									error = 1;
								}

							}
							else
							{
								DBG("JABBER:worker_process:%d: Cannot connect"
									" to the Jabber server ...\n", _pid);
								jab_send_sip_msgz(jsmsg->from, &jsmsg->to, 
									jwl->contact_h, "ERROR: Your message was"
									" not sent. Cannot connect to the Jabber"
									" server.");
								error = 1;
							}

						}
						else
						{
							DBG("JABBER:worker_process:%d: no database"
								" result\n", _pid);
							jab_send_sip_msgz(jsmsg->from, &jsmsg->to, 
								jwl->contact_h, "ERROR: Your message was not"
								" sent. You do not have permision to use the"
								" gateway.");
							error = 1;
						}
						if ((res != NULL) && (db_free_query(db_con,res) < 0))
						{
							DBG("JABBER:worker_process:%d:Error while freeing"
								" SQL result\n", _pid);
							return -1;
						}
						else
							res = NULL;
					}
				}
				else
				{
					DBG("JABBER:worker_process:%d: connection already exists"
						" for <%s> ...\n", _pid, buff);
					open_jc_update(ojc, ctime);
				}
				
				if(!error)
				{
					if(ojc->ready < get_ticks())
					{
						DBG("JABBER: worker_process:%d: SENDING AS JABBER"
							" MESSAGE ...\n", _pid);
						if(jb_send_msg(ojc->jbc, jsmsg->to.s, jsmsg->to.len,
								jsmsg->msg.s, jsmsg->msg.len)<0)
							jab_send_sip_msgz(jsmsg->from, &jsmsg->to,
								jwl->contact_h, "ERROR: Your message was not"
								" sent. Something wrong during transmition to"
								" Jabber.");
						
						jab_sipmsg_free(jsmsg);
					}
					else
					{
						DBG("JABBER:worker_process:%d:SCHEDULING THE MESSAGE."
							"\n", _pid);
						if(jc_pool_add_jmsg(jcp, jsmsg, ojc) < 0)
						{
							DBG("JABBER: worker_process:%d: SCHEDULING THE"
								" MESSAGE FAILED. Message was droped.\n",_pid);
							jab_sipmsg_free(jsmsg);
						}
					}
				}
				else
					jab_sipmsg_free(jsmsg);
			}
			else
			{ // new message from ... JABBER
				for(i = 0; i < jcp->len; i++)
				{
					if(jcp->ojc[i] != NULL && jcp->ojc[i]->jbc != NULL)
					{
						DBG("JABBER: worker_process:%d: checking socket <%d>"
							" ...\n", _pid, jcp->ojc[i]->jbc->sock);
						if(FD_ISSET(jcp->ojc[i]->jbc->sock, &mset))
						{
							if((n = read(jcp->ojc[i]->jbc->sock, recv_buff,
										sizeof(recv_buff))) > 0)
							{
								open_jc_update(jcp->ojc[i], ctime);
								
								DBG("JABBER: JMSG START ----------\n%.*s\n"
									" JABBER: JMSG END ----------\n", 
									n, recv_buff);
								
								recv_buff[n] = 0;
								if(strstr(recv_buff, "<message ") != NULL)
								{
									if(j2s_parse_jmsgx(recv_buff,n,&tjmsg)>=0)
									{
										DBG("JABBER: worker_process:%d:"
											" sending as SIP ...\n", _pid);
										buff[0] = 0;
										if(tjmsg.error.len > 0)
										{
											strcpy(buff, "{Error: ");
											if(tjmsg.errcode.len > 0)
											{
												strncat(buff, tjmsg.errcode.s, 
													tjmsg.errcode.len);
												strncat(buff, " - ", 3);
											}
											strncat(buff, tjmsg.error.s, 
												tjmsg.error.len);
											strcat(buff, ". The following"
												" message was NOT sent}: ");
										}
										strncat(buff, tjmsg.body.s, 
											tjmsg.body.len);
										if((n=xml_unescape(buff, strlen(buff),
													tbuff, 1024)) > 0)
										{
											tstr.s = tbuff;
											tstr.len = n;
											if(jab_send_sip_msg(jcp->ojc[i]->id,
													&tjmsg.from,jwl->contact_h,
													&tstr) < 0)
												DBG("JABBER:worker_process:%d:"
													" ERROR SIP MESSAGE was not"
													" sent ...\n", _pid);
											else
												DBG("JABBER:worker_process:%d:"
													" SIP MESSAGE was sent.\n",
													_pid);
										}
										else
										{
											DBG("JABBER: worker_process:%d:"
												" ERROR sending as sip: output"
												" buffer too small.\n", _pid);
										}
									}
									else
										DBG("JABBER: worker_process:%d: ERROR"
											" parsing jabber message ...\n",
											_pid);
								}
							}
							else
							{
								DBG("JABBER: worker_process:%d: ERROR -"
									" connection to jabber lost on socket <%d>"
									" ...\n", _pid, jcp->ojc[i]->jbc->sock);
								jcp->ojc[i]->expire = ltime = 0;
							}
						}
					}
				}
			}			
		} // END IF ret>0
		
		if(ret < 0)
		{
			DBG("JABBER: worker_process:%d: SIGNAL received!!!!!!!!\n", _pid);
			maxfd = pipe;
			FD_ZERO(&set);
			FD_SET(pipe, &set);
			for(i = 0; i < jcp->len; i++)
			{
				if(jcp->ojc[i] != NULL && jcp->ojc[i]->jbc != NULL)
				{
					FD_SET(jcp->ojc[i]->jbc->sock, &set);
					if( jcp->ojc[i]->jbc->sock > maxfd )
						maxfd = jcp->ojc[i]->jbc->sock;
				}
			}
		}
		
		if(ltime + wtime <= get_ticks())
		{
			ltime = get_ticks();
			DBG("JABBER: worker_process:%d: scanning for expired connection\n",
				_pid);
			for(i = 0; i < jcp->len; i++)
			{
				if((jcp->ojc[i] != NULL) && (jcp->ojc[i]->expire <= ltime))
				{
					DBG("JABBER: worker_process:%d: connection expired for"
						" <%.*s>\n", _pid, jcp->ojc[i]->id->len, 
						jcp->ojc[i]->id->s);
					
					// CLEAN JAB_WLIST
					jab_wlist_del(jwl, jcp->ojc[i]->id, _pid);
					
					FD_CLR(jcp->ojc[i]->jbc->sock, &set);
					jb_disconnect(jcp->ojc[i]->jbc);
					open_jc_free(jcp->ojc[i]);
					jcp->ojc[i] = NULL;
				}
			}
		}
				
	} // END while
	
	return 0;
}

/*****************************     ****************************************/

/**
 * init a jc_pool structure
 * - size : maximum number of the open connection to Jabber
 * - jlen : maximun size of messages queue
 * #return : pointer to the structure or NULL on error
 */
jc_pool jc_pool_init(int size, int jlen)
{
	jc_pool jcp = (jc_pool)_M_MALLOC(sizeof(t_jc_pool));
	if(jcp == NULL)
		return NULL;
	jcp->len = size;
	jcp->ojc = (open_jc*)_M_MALLOC(size*sizeof(open_jc));
	if(jcp->ojc == NULL)
	{
		_M_FREE(jcp);
		return NULL;
	}
	memset( jcp->ojc , 0, size*sizeof(open_jc) );
	jcp->jmqueue.len = jlen;
	jcp->jmqueue.size = 0;
	jcp->jmqueue.head = 1;
	jcp->jmqueue.tail = 0;
	jcp->jmqueue.jsm = (jab_sipmsg*)_M_MALLOC(jlen*sizeof(jab_sipmsg));
	if(jcp->jmqueue.jsm == NULL)
	{
		_M_FREE(jcp->ojc);
		_M_FREE(jcp);
		return NULL;
	}
	memset( jcp->jmqueue.jsm , 0, jlen*sizeof(jab_sipmsg) );
	jcp->jmqueue.ojc = (open_jc*)_M_MALLOC(jlen*sizeof(open_jc));
	if(jcp->jmqueue.ojc == NULL)
	{
		_M_FREE(jcp->jmqueue.jsm);
		_M_FREE(jcp->ojc);
		_M_FREE(jcp);
		return NULL;
	}
	memset( jcp->jmqueue.ojc , 0, jlen*sizeof(open_jc) );
	return jcp;
}

/**
 * add a new element in messages queue
 * - jcp : pointer to the Jabber connections pool structure
 * - _jsm : pointer to the message
 * - _ojc : pointer to the Jabber connection that will be used for this message
 * #return : 0 on success or <0 on error
 */
int jc_pool_add_jmsg(jc_pool jcp, jab_sipmsg _jsm, open_jc _ojc)
{
	
	if(jcp == NULL)
		return -1;
	if(jcp->jmqueue.size == jcp->jmqueue.len)
		return -2;
		
	DBG("JABBER: JC_POOL_ADD: add connection into the pool\n");
	if(++jcp->jmqueue.tail == jcp->jmqueue.len)
		jcp->jmqueue.tail = 0;
	jcp->jmqueue.size++;
	
	jcp->jmqueue.jsm[jcp->jmqueue.tail] = _jsm;
	jcp->jmqueue.ojc[jcp->jmqueue.tail] = _ojc;
	
	return 0;
}

/**
 * delete first element from messages queue
 * - jcp : pointer to the Jabber connections pool structure
 * #return : 0 on success or <0 on error
 */
int jc_pool_del_jmsg(jc_pool jcp)
{
	if(jcp == NULL)
		return -1;
	if(jcp->jmqueue.size == 0)
		return -2;
	jcp->jmqueue.size--;
	jcp->jmqueue.jsm[jcp->jmqueue.head] = NULL;
	jcp->jmqueue.ojc[jcp->jmqueue.head] = NULL;
	if(++jcp->jmqueue.head == jcp->jmqueue.len)
		jcp->jmqueue.head = 0;
	
	return 0;
}

/**
 * add a new connection in pool
 * - jcp : pointer to the Jabber connections pool structure
 * #return : 0 on success or <0 on error
 */
int jc_pool_add(jc_pool jcp, t_open_jc *jc)
{
	int i = 0;
	
	if(jcp == NULL)
		return -1;
	DBG("JABBER: JC_POOL_ADD: add connection into the pool\n");
	
	while(i < jcp->len && jcp->ojc[i] != NULL)
		i++;
	if(i >= jcp->len)
		return -1;
	jcp->ojc[i] = jc;
	
	return 0;
}

/**
 * get the jabber connection associated with 'id'
 * - jcp : pointer to the Jabber connections pool structure
 * - id : id of the Jabber connection
 * #return : pointer to the open connection to Jabber structure or NULL on error
 */
open_jc  jc_pool_get(jc_pool jcp, str *id)
{
	int i = 0;
	open_jc _ojc;
	
	if(jcp == NULL || id == NULL)
		return NULL;
	
	DBG("JABBER: JC_POOL_GET: looking for the connection of <%.*s>"
		" into the pool\n", id->len, id->s);
	while(i < jcp->len)
	{
	 	if((jcp->ojc[i]!=NULL) && (!strncmp(jcp->ojc[i]->id->s, id->s, 
				id->len)))
	 	{
	 		_ojc = jcp->ojc[i];
	 		//jcp->ojc[i] = NULL;
	 		return _ojc;
	 	}
		i++;
	}
	
	return NULL;
}

/**
 * remove the connection associated with 'id' from pool
 * - jcp : pointer to the Jabber connections pool structure
 * - id : id of the Jabber connection
 * #return : 0 on success or <0 on error
 */
int jc_pool_del(jc_pool jcp, str *id)
{
	int i = 0;
	
	if(jcp == NULL)
		return -1;
	
	DBG("JABBER: JC_POOL_DEL: removing a connection from the pool\n");
	
	while(i < jcp->len)
	{
	 	if((jcp->ojc[i]!=NULL) && (!strncmp(jcp->ojc[i]->id->s, id->s, 
			id->len)))
	 	{
	 		open_jc_free(jcp->ojc[i]);
	 		jcp->ojc[i] = NULL;
	 		break;
	 	}
		i++;
	}
	
	return 0;
}

/**
 * free a Jabber connections pool structure
 * - jcp : pointer to the Jabber connections pool structure
 */
void jc_pool_free(jc_pool jcp)
{
	int i;
	if(jcp == NULL)
		return;
	DBG("JABBER: JC_POOL_FREE ----------\n");
	if(jcp->ojc != NULL)
	{
		for(i=0; i<jcp->len; i++)
		{
			if(jcp->ojc[i] != NULL)
				open_jc_free(jcp->ojc[i]);
		}
		_M_FREE(jcp->ojc);
	}
	if(jcp->jmqueue.jsm != NULL)
		_M_FREE(jcp->jmqueue.jsm);
	if(jcp->jmqueue.ojc != NULL)
		_M_FREE(jcp->jmqueue.ojc);
	
	_M_FREE(jcp);
}

/*****************************     ****************************************/

/**
 * create a open connection to Jabber
 * - id : id of the connection
 * - jbc : pointer to Jabber connection
 * - cache_time : life time of the connection
 * - delay_time : time needed to became an active connection
 * #return : pointer to the structure or NULL on error
 */
open_jc open_jc_create(str *id, jbconnection jbc, int cache_time, 
				int delay_time)
{
	open_jc ojc;
	int t;
	DBG("JABBER: OPEN_JC_CREATE ----------\n");
	ojc = (open_jc)_M_MALLOC(sizeof(t_open_jc));
	if(ojc == NULL)
		return NULL;
	ojc->id = id;
	t = get_ticks();
	ojc->expire = t + cache_time;
	ojc->ready = t + delay_time;
	ojc->jbc = jbc;
	return ojc;
}

/**
 * update the life time of the connection
 * - ojc : pointer to the open connection
 * - cache_time : number of seconds to keep the connection open
 * #return : 0 on success or <0 on error
 */
int open_jc_update(open_jc ojc, int cache_time)
{
	if(ojc == NULL)
		return -1;
	DBG("JABBER: OPEN_JC_UPDATE ----------\n");
	ojc->expire = get_ticks() + cache_time;
	return 0;	
}

/**
 * free an open connection structure
 * - ojc : pointer to the open connection
 * > ojc->id will not be FREE
 */
void open_jc_free(open_jc ojc)
{
	if(ojc == NULL)
		return;
	DBG("JABBER: OPEN_JC_FREE ----------\n");
	if(ojc->jbc != NULL)
	{
		//jb_disconnect(ojc->jbc);
		jb_free_jbconnection(ojc->jbc);
	}
	//if(ojc->id != NULL)
	//	_M_FREE(ojc->id);
	_M_FREE(ojc);
	DBG("JABBER: OPEN_JC_FREE ---END---\n");
}

