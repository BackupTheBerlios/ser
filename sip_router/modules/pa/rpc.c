#include "../../error.h"
#include "../../parser/parse_event.h"
#include "pdomain.h"
#include "dlist.h"
#include <cds/sstr.h>
#include <time.h>
#include "qsa_interface.h"
#include "pa_mod.h"
#include <cds/logger.h>

#include <presence/pidf.h>
#include "publish.h"

extern dlist_t* root; /* FIXME ugly !!!!! */

#define rpc_lf(rpc, c)	rpc->add(c, "s","")

static void trace_tuple(presence_tuple_t *t, rpc_t* rpc, void* c) {
	presence_note_t *n;
	
	rpc->printf(c, "    %.*s contact=\'%.*s\' exp=%u status=%d published=%d (id=%.*s)", 
				FMT_STR(t->id), FMT_STR(t->contact), t->expires - time(NULL),
				(int)t->state, t->is_published, FMT_STR(t->published_id));
	rpc_lf(rpc, c);
	
	rpc->printf(c, "      notes:");
	n = t->notes;
	while (n) {
		rpc->printf(c, " \'%.*s\'", FMT_STR(n->value));
		n = n->next;
	}
	rpc->printf(c, "");
	rpc_lf(rpc, c);
}

static void trace_presentity(presentity_t *p, rpc_t* rpc, void* c)
{
	watcher_t *w;
	presence_tuple_t *t;
	internal_pa_subscription_t *iw;
	pa_presence_note_t *n;
	pa_person_element_t *ps;
	
	rpc->printf(c, "* %.*s (uid=%.*s)", FMT_STR(p->uri), FMT_STR(p->uuid));
	rpc_lf(rpc, c);
	
	rpc->printf(c, " - tuples:");
	rpc_lf(rpc, c);
	t = p->tuples;
	while (t) {		
		trace_tuple(t, rpc, c);
		t = t->next;
	}
	
	rpc->printf(c, " - watchers:");
	rpc_lf(rpc, c);
	w = p->watchers;
	while (w) {
		rpc->printf(c, "    %.*s status=%d exp=%u", 
				FMT_STR(w->uri), (int)w->status, w->expires - time(NULL));
		rpc_lf(rpc, c);
		w = w->next;
	}
	
	rpc->printf(c, " - winfo watchers:");
	rpc_lf(rpc, c);
	w = p->winfo_watchers;
	while (w) {
		rpc->printf(c, "    %.*s status=%d exp=%u", 
				FMT_STR(w->uri), (int)w->status, w->expires - time(NULL));
		rpc_lf(rpc, c);
		w = w->next;
	}
	
	rpc->printf(c, " - internal watchers:");
	rpc_lf(rpc, c);
	iw = p->first_qsa_subscription;
	while (iw) {
		rpc->printf(c, "     %.*s %d", 
				FMT_STR(iw->subscription->subscriber_id), (int)iw->status);
		rpc_lf(rpc, c);
		iw = iw->next;
	}
	
	rpc->printf(c, " - notes:");
	rpc_lf(rpc, c);
	n = p->notes;
	while (n) {
		rpc->printf(c, "     %.*s (%.*s) exp=%s", 
				FMT_STR(n->note), FMT_STR(n->lang), ctime(&n->expires));
		n = n->next;
	}
	rpc_lf(rpc, c);
	
	rpc->printf(c, " - person elements:");
	rpc_lf(rpc, c);
	ps = p->person_elements;
	while (ps) {
		rpc->printf(c, "     %.*s exp=%s %.*s", 
				FMT_STR(ps->id), ctime(&ps->expires));
		rpc_lf(rpc, c);
		rpc->printf(c, "     %.*s", FMT_STR(ps->person));
		rpc_lf(rpc, c);
		ps = ps->next;
	}
	rpc_lf(rpc, c);
}

static void trace_dlist(dlist_t *dl, rpc_t* rpc, void* c)
{
	presentity_t *p;

	if (!dl) return;
	if (!dl->d) return;

	lock_pdomain(dl->d);
	
	rpc->add(c, "S", dl->d->name);
	p = dl->d->first;
	while (p) {
		trace_presentity(p, rpc, c);
		p = p->next;
	}
	
	unlock_pdomain(dl->d);
}


static const char* rpc_trace_doc[] = {
	"Display internal data structure.",  /* Documentation string */
	0                                    /* Method signature(s) */
};

static void rpc_trace(rpc_t* rpc, void* c)
{
	dlist_t *dl;

	dl = root;
	while (dl) {
		trace_dlist(dl, rpc, c);
		dl = dl->next;
	}
	
	rpc->send(c);
	
}

static int grant_watcher(presentity_t *p, watcher_t *w)
{
	int changed = 0;
	switch (w->status) {
		case WS_PENDING:
		case WS_REJECTED:
			w->status = WS_ACTIVE;
			changed = 1;
			break;
		case WS_PENDING_TERMINATED:
			w->status = WS_TERMINATED;
			changed = 1;
			break;
			
		default: break;
	}

	if (changed) {
		w->flags |= WFLAG_SUBSCRIPTION_CHANGED;
		if (w->event_package != EVENT_PRESENCE_WINFO) 
			p->flags |= PFLAG_WATCHERINFO_CHANGED;
	}
	
	return 0;
}

static int grant_internal_watcher(presentity_t *p, internal_pa_subscription_t *w)
{
	int changed = 0;
	switch (w->status) {
		case WS_PENDING:
		case WS_REJECTED:
			w->status = WS_ACTIVE;
			changed = 1;
			break;
		case WS_PENDING_TERMINATED:
			w->status = WS_TERMINATED;
			changed = 1;
			break;
			
		default: break;
	}

	if (changed) {
		/* w->flags |= WFLAG_SUBSCRIPTION_CHANGED; */
		notify_internal_watcher(p, w);
		p->flags |= PFLAG_WATCHERINFO_CHANGED;
	}
	
	return 0;
}

static int grant_watchers(presentity_t *p, str *wuri)
{
	watcher_t *w;
	internal_pa_subscription_t *iw;
	
	w = p->watchers;
	while (w) {
		if (str_case_equals(&w->uri, wuri) == 0) grant_watcher(p, w);
		w = w->next;
	}
	
	iw = p->first_qsa_subscription;
	while (iw) {
		if (str_case_equals(&iw->subscription->subscriber_id, wuri) == 0) 
			grant_internal_watcher(p, iw);
		iw = iw->next;
	}

	return 0;
}


static const char* rpc_authorize_doc[] = {
	"Authorize watcher.",  /* Documentation string */
	0                     /* Method signature(s) */
};

static void rpc_authorize(rpc_t* rpc, void* c)
{
	pdomain_t *d;
	presentity_t *p;
	
	str pstr, wstr, uid;
	char* domain;

	if (rpc->scan(c, "sSS", &domain, &pstr, &wstr) < 3) {
		rpc->fault(c, 400, "Invalid parameter value");
		return;
	}

	if (find_pdomain(domain, &d) != 0) {
		rpc->fault(c, 400, "Unknown domain '%s'\n", domain);
		return;
	}

	if (pres_uri2uid(&uid, &pstr) != 0) {
		rpc->fault(c, 400, "Unable to convert '%.*s' to UID\n", pstr.len, pstr.s);
		return;
	}
	
	lock_pdomain(d);
	if (find_presentity_uid(d, &pstr, &p) != 0) {
		rpc->fault(c, 400, "Presentity '%.*s' not found\n", pstr.len, pstr.s);
		unlock_pdomain(d);
		str_free_content(&uid);
		return;
	}
	grant_watchers(p, &wstr);
	unlock_pdomain(d);
	str_free_content(&uid);
}

static void rpc_pa_publish(rpc_t* rpc, void* c)
{
	pdomain_t *d;
	presentity_t *p;
	presentity_info_t *pi = NULL;
	str pstr, uid, doc;
	char* domain;
	str etag = STR_NULL;
	time_t expires = time(NULL);
	int exp_sec = 0;
	int res;
	void *st;
	
	res = rpc->scan(c, "sSSd", &domain, &pstr, &doc, &exp_sec);
	/* TODO: args = domain, uri, presence doc, expires, etag (for republishing) */
	if (res < 4) {
		rpc->fault(c, 400, "Invalid parameter value (%d)", res);
		return;
	}
	if (rpc->scan(c, "S", &etag) < 1) etag.len = 0;
	expires += exp_sec;

	if (find_pdomain(domain, &d) != 0) {
		rpc->fault(c, 400, "Unknown domain '%s'\n", domain);
		return;
	}

	if (pres_uri2uid(&uid, &pstr) != 0) {
		rpc->fault(c, 400, "Unable to convert '%.*s' to UID\n", pstr.len, pstr.s);
		return;
	}
	
	lock_pdomain(d);
	res = find_presentity_uid(d, &uid, &p);
	
	if (res > 0) res = new_presentity(d, &pstr, &uid, &p);
	if (res < 0) {
		rpc->fault(c, 400, "Can't create/find presentity '%.*s'\n", pstr.len, pstr.s);
		unlock_pdomain(d);
		str_free_content(&uid);
		return;
	}
	str_free_content(&uid);

	if (parse_pidf_document(&pi, doc.s, doc.len) != 0) {
		rpc->fault(c, 400, "Can't parse presence document (not PIDF?)\n");
		unlock_pdomain(d);
		return;
		
	}

	if (pi) {
		if (process_published_presentity_info(p, pi, &etag, expires) != 0) {
			free_presentity_info(pi);
			rpc->fault(c, 400, "Can't publish\n");
			unlock_pdomain(d);
			return;
		}
	}

	free_presentity_info(pi);
	unlock_pdomain(d);
	
	if (rpc->add(c, "{", &st) < 0) return;
	rpc->struct_add(st, "S", "etag", &etag);
	rpc->send(c);
}

static const char* rpc_pa_publish_doc[] = {
	"Publish presence document.",  /* Documentation string */
	0                     /* Method signature(s) */
};

/* 
 * RPC Methods exported by this module 
 */
rpc_export_t pa_rpc_methods[] = {
	{"pa.authorize", rpc_authorize,      rpc_authorize_doc,  0},
	{"pa.trace",     rpc_trace,          rpc_trace_doc,      0},
	{"pa.publish",   rpc_pa_publish,     rpc_pa_publish_doc, 0},
	{0, 0, 0, 0}
};
