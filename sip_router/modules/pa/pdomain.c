/*
 * Presence Agent, domain support
 *
 * $Id: pdomain.c,v 1.27 2006/02/03 16:25:58 kubartv Exp $
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
 *  2003-03-11  converted to the new locking scheme: locking.h (andrei)
 */


#include "pdomain.h"
#include "paerrno.h"
#include "presentity.h"
#include "../../ut.h"
#include "../../dprint.h"
#include "../../mem/shm_mem.h"
#include <cds/logger.h>
#include "pa_mod.h"

/*
 * Hash function
 */
static inline int hash_func(pdomain_t* _d, char* _s, int _l)
{
	int res = 0, i;
	
	for(i = 0; i < _l; i++) {
		res += _s[i];
	}
	
	return res % _d->size;
}


/*
 * Create a new domain structure
 * _n is pointer to str representing
 * name of the domain, the string is
 * not copied, it should point to str
 * structure stored in domain list
 * _s is hash table size
 */
int new_pdomain(str* _n, int _s, pdomain_t** _d, register_watcher_t _r, unregister_watcher_t _u)
{
	int i;
	pdomain_t* ptr;
	
	ptr = (pdomain_t*)shm_malloc(sizeof(pdomain_t));
	if (!ptr) {
		paerrno = PA_NO_MEMORY;
		LOG(L_ERR, "new_pdomain(): No memory left\n");
		return -1;
	}
	memset(ptr, 0, sizeof(pdomain_t));
	
	ptr->table = (hslot_t*)shm_malloc(sizeof(hslot_t) * _s);
	if (!ptr->table) {
		paerrno = PA_NO_MEMORY;
		LOG(L_ERR, "new_pdomain(): No memory left 2\n");
		shm_free(ptr);
		return -2;
	}

	ptr->name = _n;
	
	for(i = 0; i < _s; i++) {
		init_slot(ptr, &ptr->table[i]);
	}

	ptr->size = _s;
	lock_init(&ptr->lock);
	ptr->users = 0;
	ptr->expired = 0;
	
	ptr->reg = _r;
	ptr->unreg = _u;
	*_d = ptr;
	return 0;
}


/*
 * Free all memory allocated for
 * the domain
 */
void free_pdomain(pdomain_t* _d)
{
	int i;
	
	lock_pdomain(_d);
	if (_d->table) {
		for(i = 0; i < _d->size; i++) {
			deinit_slot(_d->table + i);
		}
		shm_free(_d->table);
	}
	unlock_pdomain(_d);

        shm_free(_d);
}

int timer_pdomain(pdomain_t* _d)
{
	struct presentity* presentity, *t;

	lock_pdomain(_d);

	presentity = _d->first;

	while(presentity) {
		if (timer_presentity(presentity) < 0) {
			LOG(L_ERR, "timer_pdomain(): Error in timer_pdomain\n");
			unlock_pdomain(_d);
			return -1;
		}
		
		/* Remove the entire record
		 * if it is empty
		 */
		if ( (!presentity->watchers) && 
				(!presentity->winfo_watchers) && 
				(!presentity->tuples) &&
				(!presentity->notes) &&
				(!presentity->first_qsa_subscription)) {
			LOG(L_DBG, "timer_pdomain(): removing empty presentity\n");
			t = presentity;
			presentity = presentity->next;
			release_presentity(t);
		} else {
			presentity = presentity->next;
		}
	}
	
	unlock_pdomain(_d);
	return 0;
}


static int in_pdomain = 0; /* this only works with single or multiprocess execution model, but not multi-threaded */

/*
 * Get lock if this process does not already have it
 */
void lock_pdomain(pdomain_t* _d)
{
	DBG("lock_pdomain\n");
	if (!in_pdomain++)
	     lock_get(&_d->lock);
}


/*
 * Release lock
 */
void unlock_pdomain(pdomain_t* _d)
{
	DBG("unlock_pdomain\n");
	in_pdomain--;
	if (!in_pdomain)
	     lock_release(&_d->lock);
}

/*
 * Find a presentity in domain according to uid
 */
int find_presentity_uid(pdomain_t* _d, str* uid, struct presentity** _p)
{
	int sl, i;
	struct presentity* p;
	int res = 1;

	if (!uid) return -1;

	sl = hash_func(_d, uid->s, uid->len);
	
	p = _d->table[sl].first;
	
	for(i = 0; i < _d->table[sl].n; i++) {
		if ((p->uuid.len == uid->len) && !memcmp(p->uuid.s, uid->s, uid->len)) {
			*_p = p;
			res = 0;
			break;
		}
		p = p->next;
	}
	
	return res;   /* Nothing found */
}

void callback(str* _user, str *_contact, int state, void* data);

void add_presentity(pdomain_t* _d, struct presentity* _p)
{
	int sl;

	LOG(L_DBG, "add_presentity _p=%p p_uri=%.*s\n", _p, _p->uri.len, _p->uri.s);

	sl = hash_func(_d, _p->uuid.s, _p->uuid.len);

	slot_add(&_d->table[sl], _p, &_d->first, &_d->last);

	if (use_callbacks) {
		DBG("! registering callback to %.*s, %p\n", _p->uuid.len, _p->uuid.s,_p);
		_d->reg(&_p->uri, &_p->uuid, (void*)callback, _p);
	}
}


void remove_presentity(pdomain_t* _d, struct presentity* _p)
{
	if (use_callbacks) {
		DBG("! unregistering callback to %.*s, %p\n", _p->uuid.len, _p->uuid.s,_p);
		_d->unreg(&_p->uri, &_p->uuid, (void*)callback, _p);
		DBG("! unregistered callback to %.*s, %p\n", _p->uuid.len, _p->uuid.s,_p);
	}
	
	LOG(L_DBG, "remove_presentity _p=%p p_uri=%.*s\n", _p, _p->uri.len, _p->uri.s);
	slot_rem(_p->slot, _p, &_d->first, &_d->last);

	/* remove presentity from database */
}

