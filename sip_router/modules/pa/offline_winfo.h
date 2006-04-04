#ifndef __OFFLINE_WINFO_H
#define __OFFLINE_WINFO_H

#include "pa_mod.h"

typedef struct _offline_winfo_t {
	time_t created; /* date and time of this info creation */
	time_t expires; /* date and time of this info expiration */
	int index;      /* key generated by database - it is used for removing winfos from DB */
	str uid;     /* presentity identification */
	str watcher; /* watcher's uri */
	str events;  /* event package */
	str domain;  /* domain */
	str status;  /* status of subscription */
	struct _offline_winfo_t *next; /* for linking infos together */
	char buffer[1];
} offline_winfo_t;

int store_offline_winfo(struct sip_msg* _m, char* _domain, char* _str);
/* send offline winfo as regular watcher info NOTIFY if possible 
 * (for presentity got from get_to) */
int dump_offline_winfo(struct sip_msg* _m, char* _domain, char* _s2);

/* expiration time in seconds */
extern int offline_winfo_expiration;

void offline_winfo_timer(unsigned int ticks, void* param);

int send_winfo_notify_offline(struct presentity* _p, 
		struct watcher* _w, 
		offline_winfo_t *info, 
		transaction_cb completion_cb, void* cbp);
	
#endif
