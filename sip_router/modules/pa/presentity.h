/*
 * Presence Agent, presentity structure and related functions
 *
 * $Id: presentity.h,v 1.36 2006/07/10 12:39:17 kubartv Exp $
 *
 * Copyright (C) 2001-2003 FhG Fokus
 * Copyright (C) 2004 Jamey Hicks
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

#ifndef PRESENTITY_H
#define PRESENTITY_H

#include "../../str.h"
#include "../tm/dlg.h"
#include "watcher.h"
#include "hslot.h"
#include "pstate.h"
#include "trace.h"

#include <xcap/pres_rules.h>
#include <cds/msg_queue.h>
#include <presence/notifier.h>
#include <presence/subscriber.h>
#include <presence/pres_doc.h>

enum prescaps {
	PRESCAP_AUDIO = (1 << 0),
	PRESCAP_VIDEO = (1 << 1),
	PRESCAP_TEXT = (1 << 2),
	PRESCAP_APPLICATION = (1 << 3)
};
extern const char *prescap_names[];

#define TUPLE_STATUS_STR_LEN 128
#define TUPLE_LOCATION_LOC_LEN 128
#define TUPLE_LOCATION_SITE_LEN 32
#define TUPLE_LOCATION_FLOOR_LEN 32
#define TUPLE_LOCATION_ROOM_LEN 64
#define TUPLE_LOCATION_PACKET_LOSS_LEN 32
#define TUPLE_ID_STR_LEN (32)

typedef struct presence_tuple {
	str id;
	str contact;
	str status;
	enum prescaps prescaps;
	double priority;
	time_t expires;
	pstate_t state;
	struct presence_tuple *next;
	struct presence_tuple *prev;
	char status_buf[TUPLE_STATUS_STR_LEN];
	char id_buf[TUPLE_ID_STR_LEN];
	int is_published;	/* 1 for published tuples - these are stored into DB */

	str etag;	/* etag for published tuples */
	str published_id;	/* tuple id used for publish */
	presence_note_t *notes; /* notes for this tuple */
} presence_tuple_t;

typedef struct pa_presence_note {
	str etag; /* published via this etag */
	str note; /* note string */
	str lang; /* language, may be empty */
	time_t expires; /* note expires on ... */
	str dbid; /* id for database ops - needed for removing expired notes */
	struct pa_presence_note *prev, *next; /* linking members */
} pa_presence_note_t;

typedef struct _pa_person_element_t {
	str etag; /* published via this etag */
	str person; /* person element */
	str id; /* id */
	time_t expires; /* expires on ... */
	str dbid; /* id for database ops - needed for removing expired */
	struct _pa_person_element_t *prev, *next; /* linking members */
} pa_person_element_t;

typedef struct {
	str user;
	str contact;
	pstate_t state;
} tuple_change_info_t;

struct pdomain;

typedef enum pflag {
	PFLAG_PRESENCE_CHANGED=1,
	PFLAG_WATCHERINFO_CHANGED=2
} pflag_t;

typedef struct _internal_pa_subscription_t {
	struct _internal_pa_subscription_t *prev, *next;
	watcher_status_t status;
	qsa_subscription_t *subscription;
	/* msg_queue_t *dst;
	 * str_t package; 
	 * str_t watcher_uri; */
} internal_pa_subscription_t;


typedef struct presentity {
	str uri;      /* URI of presentity - doesn't change for the presentity's life */
	int presid;   /* presid of the record in the presentity table */
	int ref_cnt;  /* reference counter - don't remove if > 1 */
	presence_tuple_t *tuples;
	watcher_t* watchers;     /* List of watchers */
	watcher_t* winfo_watchers;  /* Watchers subscribed to winfo */
	pflag_t flags;
	struct pdomain *pdomain; 
	struct presentity* next; /* Next presentity */
	struct presentity* prev; /* Previous presentity in list */
	struct hslot* slot;      /* Hash table collision slot we belong to */
	
	internal_pa_subscription_t *first_qsa_subscription, *last_qsa_subscription;
	presence_rules_t *authorization_info;
	xcap_query_params_t xcap_params; /* doesn't change for the presentity's life */
	time_t auth_rules_refresh_time;
	
	msg_queue_t mq;	/* message queue supplying direct usrloc callback processing */
	
	pa_presence_note_t *notes;
	pa_person_element_t *person_elements;
	str uuid; /* use after usrloc uuid-zation - callbacks are 
				 registered to this,  - doesn't change for 
				 the presentity's life  */

	/* data for internal subscriptions to presence */
	/* reduces memory allocation count */
	qsa_subscription_data_t presence_subscription_data;
	qsa_subscription_t *presence_subscription;
} presentity_t;

/*
 * Create a new presentity
 * (message is given due to XCAP parameters)
 */
int new_presentity(struct pdomain *pdomain, str* _uri, str *uid, 
		xcap_query_params_t *xcap_params, presentity_t** _p);


/*
 * Free all memory associated with a presentity
 */
void free_presentity(presentity_t* _p);

/*
 * Free all memory associated with a presentity and remove it from DB
 */
void release_presentity(presentity_t* _p);

/*
 * Sync presentity to db if db is in use
 */
int db_update_presentity(presentity_t* _p);

/*
 * Run a timer handler on the presentity
 */
int timer_presentity(presentity_t* _p);


/*
 * Create a new presence_tuple
 */
int new_presence_tuple(str* _contact, time_t expires, presence_tuple_t ** _t, int is_published);

/*
 * Find a presence_tuple for contact _contact on presentity _p - only registered contacts !
 */
int find_registered_presence_tuple(str* _contact, presentity_t *_p, presence_tuple_t ** _t);
int find_presence_tuple_id(str* id, presentity_t *_p, presence_tuple_t ** _t);
void add_presence_tuple(presentity_t *_p, presence_tuple_t *_t);
void remove_presence_tuple(presentity_t *_p, presence_tuple_t *_t);

int db_update_presence_tuple(presentity_t *_p, presence_tuple_t *t, int update_notes);
int db_update_person_element(presentity_t *p, pa_person_element_t *n);
int db_update_pres_note(presentity_t *p, pa_presence_note_t *n);

void set_tuple_published(presentity_t *p, presence_tuple_t *t);

/*
 * Free all memory associated with a presence_tuple
 */
void free_presence_tuple(presence_tuple_t * _t);



/*
 * Add a watcher to the watcher list
 */
int add_watcher(presentity_t* _p, struct watcher* _w);


/*
 * Remove a watcher from the watcher list
 */
int remove_watcher(presentity_t* _p, watcher_t* _w);


/*
 * Find a watcher on the watcher list
 */
int find_watcher(presentity_t* _p, str* _uri, int _et, struct watcher** _w);

/*
 * Find a watcher in the list via dialog identifier
 */
int find_watcher_dlg(struct presentity* _p, dlg_id_t *dlg_id, int _et, watcher_t** _w);

/*
 * Notify all watchers on the list
 */
int notify_watchers(presentity_t* _p);


/*
 * Add a watcher to the winfo watcher list
 */
int add_winfo_watcher(presentity_t* _p, struct watcher* _w);


/*
 * Remove a watcher from the winfo watcher list
 */
int remove_winfo_watcher(presentity_t* _p, watcher_t* _w);

/* removes winfo/normal watcher if expired */
void remove_watcher_if_expired(presentity_t* _p, watcher_t *w);

/*
 * Notify all winfo watchers in the list
 */
int notify_winfo_watchers(presentity_t* _p);

int db_remove_presentity(presentity_t* presentity);

/* helper functions */

void free_tuple_change_info_content(tuple_change_info_t *i);

/* 
 * gets UID from message (using get_to_uid) 
 * and duplicates it into shared memory 
 */
int get_presentity_uid(str *uid_dst, struct sip_msg *m);

/* 
 * converts uri to uid (uid is allocated in shm)
 * used by internal subscriptions and fifo commands 
 */
int pres_uri2uid(str_t *uid_dst, const str_t *uri);

/* presence note functions */
void add_pres_note(presentity_t *_p, pa_presence_note_t *n);
void free_pres_note(pa_presence_note_t *n);
void remove_pres_note(presentity_t *_p, pa_presence_note_t *n);
int remove_pres_notes(presentity_t *p, str *etag);
pa_presence_note_t *create_pres_note(str *etag, str *note, str *lang, time_t expires, str *dbid);
int db_read_notes(presentity_t *p, db_con_t* db);
int db_remove_pres_notes(presentity_t *p); /* remove all notes for presentity */

/* person element functions */
void add_person_element(presentity_t *_p, pa_person_element_t *n);
void free_person_element(pa_person_element_t *n);
void remove_person_element(presentity_t *_p, pa_person_element_t *n);
int remove_person_elements(presentity_t *p, str *etag);
pa_person_element_t *create_person_element(str *etag, str *element, str *id, time_t expires, str *dbid);
int db_read_person_elements(presentity_t *p, db_con_t* db);
int db_remove_person_elements(presentity_t *p); /* remove all notes for presentity */

/* tuple note functions */
int db_read_tuple_notes(presentity_t *p, presence_tuple_t *t, db_con_t* db);
int db_remove_tuple_notes(presentity_t *p, presence_tuple_t *t); /* remove all notes for tuple */
/* removes all notes for all tuples */
int db_remove_all_tuple_notes(presentity_t *p);
int db_add_tuple_notes(presentity_t *p, presence_tuple_t *t); /* add all notes for tuple into DB */
int db_update_tuple_notes(presentity_t *p, presence_tuple_t *t);
/* adds note to tuple in memory, not in DB (use update)! */
void add_tuple_note_no_wb(presence_tuple_t *t, presence_note_t *n);
/* frees all notes for given tuple (in memory only, not DB) */
void free_tuple_notes(presence_tuple_t *t);

struct pdomain;
int pdomain_load_presentities(struct pdomain *pdomain);



/* set authorization rules for presentity
 * ! call from locked region only ! */
int set_auth_rules(presentity_t *p, presence_rules_t *new_auth_rules);

/* how often refresh authorization rules (xcap change events are 
 * not implemented yet!) */
extern int auth_rules_refresh_time;

#endif /* PRESENTITY_H */
