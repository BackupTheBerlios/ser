/*
 * $Id: m_sem.h,v 1.1 2002/07/04 11:14:18 daniel Exp $
 *
 * JABBER module
 *
 */

#ifndef _M_SEM_H_
#define _M_SEM_H_


#include <sys/types.h>

int init_sem(key_t, int);
int init_mutex(key_t);
void rm_sem(int);
void semcall(int, int);
void P(int);
void V(int);
void mutex_lock(int);
void mutex_unlock(int);

#endif
