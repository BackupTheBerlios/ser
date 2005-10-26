#include "rl_subscription.h"
#include "rls_mod.h"
#include <cds/dstring.h>
#include <cds/list.h>
#include <cds/logger.h>
#include "result_codes.h"
#include "rlmi_doc.h"
#include <xcap/resource_list.h>

#include "../../str.h"
#include "../../dprint.h"
#include "../../mem/mem.h"
#include "../../mem/shm_mem.h"
#include "../../lock_alloc.h"
#include "../../ut.h"
#include "../../parser/hf.h"
#include "../../parser/parse_from.h"
#include "../../data_lump_rpl.h"

/* shared structure holding the data */
typedef struct {
	rl_subscription_t *first;
	rl_subscription_t *last;
	/* hash, ... */
} rls_data_t;

static rls_data_t *rls = NULL;
static gen_lock_t *rls_mutex = NULL;
subscription_manager_t *rls_manager = NULL;

#define METHOD_NOTIFY "NOTIFY"
#define METHOD_NOTIFY_L (sizeof(METHOD_NOTIFY) - 1)

/************* subscription callback functions ************/

static int send_notify_cb(struct _subscription_data_t *s)
{
	if (s) 
		rls_generate_notify((rl_subscription_t *)s->usr_data, 1);
	return 0;
}

static int terminate_subscription_cb(struct _subscription_data_t *s)
{
	if (s) rls_free((rl_subscription_t*)s->usr_data);
	return 0;
}

static authorization_result_t authorize_subscription_cb(struct _subscription_data_t *s)
{
/*	return auth_rejected; */
	/* TODO: better authorization function :-) */
	return auth_granted;
}

/************* global functions ************/

void rls_lock()
{
	/* FIXME: solve locking more efficiently - locking whole RLS in 
	 * all cases of manipulating internal structures is not good
	 * solution */
	lock_get(rls_mutex);
}

void rls_unlock()
{
	lock_release(rls_mutex);
}

int rls_init()
{
	rls = (rls_data_t*)shm_malloc(sizeof(rls_data_t));
	if (!rls) {
		LOG(L_ERR, "rls_init(): memory allocation error\n");
		return -1;
	}
	rls->first = NULL;
	rls->last = NULL;
	
	rls_mutex = lock_alloc();
	if (!rls_mutex) {
		LOG(L_ERR, "rls_init(): Can't initialize mutex\n");
		return -1;
	}
	lock_init(rls_mutex);

	rls_manager = sm_create(send_notify_cb, 
			terminate_subscription_cb, 
			authorize_subscription_cb,
			rls_mutex,
			rls_min_expiration,	/* min expiration time in seconds */
			rls_max_expiration, /* max expiration time in seconds */
			rls_default_expiration /* default expiration time in seconds */
			);
	
	return 0;
}

int rls_destroy()
{
	/* FIXME: destroy the whole rl_subscription list */
	/* sm_destroy(rls_manager); */
	return 0;
}

/************* Helper functions for RL subscription manipulation ************/

static int create_virtual_subscriptions(struct sip_msg *m, rl_subscription_t *ss, const char *xcap_root)
{
	flat_list_t *e, *flat = NULL;
	xcap_query_t xcap;
	virtual_subscription_t *vs;
	int res;
	str s;
	
	/* TODO: create virtual subscriptions using Accept headers ... (for remote subscriptions) */
	
	/* XCAP query */
	memset(&xcap, 0, sizeof(xcap));
	TRACE_LOG("rli_create_content(): doing XCAP query\n");
	res = get_rls(xcap_root, rls_get_uri(ss), &xcap, 
			rls_get_package(ss), &flat);
	if (res != RES_OK) return res;

	/* go through flat list and find/create virtual subscriptions */
	e = flat;
	while (e) {
		s.s = e->uri;
		if (s.s) s.len = strlen(s.s);
		else s.len = 0;

		res = vs_create(&s, rls_get_package(ss), &vs, e->names, ss);
		
		if (res != RES_OK) return res;
		DOUBLE_LINKED_LIST_ADD(ss->first_vs, ss->last_vs, vs);

		e = e->next;
	}

	TRACE_LOG("rli_create_content(): freeing flat list\n");
	free_flat_list(flat);
	return RES_OK;
}
/************* RL subscription manipulation function  ************/

int rls_create_subscription(struct sip_msg *m, rl_subscription_t **dst, const char *xcap_root)
{
	rl_subscription_t *s;
	int res;

	if (!dst) return RES_INTERNAL_ERR;
	*dst = NULL;

	/* FIXME: test if there is a "Require: eventlist" header - if not,
	 * it is probbably not a message for RLS! */
	/* FIXME: test if Accept = multipart/related, application/rlmi+xml! */
	
	s = (rl_subscription_t*)shm_malloc(sizeof(rl_subscription_t));
	if (!s) {
		LOG(L_ERR, "rls_create_new(): can't allocate memory\n");
		return RES_MEMORY_ERR;
	}
	s->subscription.status = subscription_uninitialized;
	s->doc_version = 0;
	s->changed = 0;
	s->first_vs = NULL;
	s->last_vs = NULL;

	res = sm_init_subscription_nolock(rls_manager, &s->subscription, m);
	if (res != RES_OK) {
		rls_free(s);
		return res;
	}

	/* store pointer to this RL subscription as user data of (low level) subscription */
	s->subscription.usr_data = s; 

/*	res = set_rls_info(m, s, xcap_root);
	if (res != 0) {
		rls_free(s);
		return res;
	}*/
	
	res = create_virtual_subscriptions(m, s, xcap_root);
	if (res != 0) {
		rls_free(s);
		return res;
	}

	*dst = s;
	return RES_OK;
}

int rls_find_subscription(str *from_tag, str *to_tag, str *call_id, rl_subscription_t **dst)
{
	subscription_data_t *s;
	int res;

	*dst = NULL;
	res = sm_find_subscription(rls_manager, from_tag, to_tag, call_id, &s);
	if ((res == RES_OK) && (s)) {
		if (!s->usr_data) {
			LOG(L_ERR, "found subscription without filled usr_data\n");
			return RES_INTERNAL_ERR;
		}
		else {
			*dst = (rl_subscription_t*)s->usr_data;
			return RES_OK;
		}
	}
	return RES_NOT_FOUND;
}

int rls_refresh_subscription(struct sip_msg *m, rl_subscription_t *s)
{
	int res;

	if (!s) return RES_INTERNAL_ERR;
	res = sm_refresh_subscription_nolock(rls_manager, &s->subscription, m);

	return res;
}

void rls_free(rl_subscription_t *s)
{
	virtual_subscription_t *vs, *nvs;
	
	if (!s) return;
	
	sm_release_subscription_nolock(rls_manager, &s->subscription);
	vs = s->first_vs;
	while (vs) {
		nvs = vs->next;
		vs_free(vs);
		vs = nvs;
	}
	shm_free(s);
}

/* void rls_notify_cb(struct cell* t, struct sip_msg* msg, int code, void *param) */
void rls_notify_cb(struct cell* t, int type, struct tmcb_params* params)
{
	rl_subscription_t *s = NULL;

	if (!params) return;

	if (params->param) s = (rl_subscription_t *)*(params->param);

	if (params->code >= 300) { /* what else can we do with 3xx ? */
		LOG(L_ERR, "rls_notify_cb(): %d response on NOTIFY - removing subscription %p\n", params->code, s);

		rls_lock();
		if (s) rls_free(s);
		rls_unlock();
	}
}

int rls_generate_notify(rl_subscription_t *s, int full_info)
{
	/* !!! the main mutex must be locked here !!! */
	int res;
	str doc;
	dstring_t dstr;
	str headers, content_type;
	static str method = {s: METHOD_NOTIFY, len: METHOD_NOTIFY_L};
	dlg_t *dlg;
	
	if (!s) {
		LOG(L_ERR, "rls_generate_notify(): called with <null> subscription\n");
		return -1;
	}
	
	dlg = s->subscription.dialog;
	if (!dlg) return -1;

	str_clear(&doc);
	str_clear(&content_type);
	if (sm_subscription_pending(&s->subscription) != 0) {
		/* create the document only for non-pending subscriptions */
		if (create_rlmi_document(&doc, &content_type, s, full_info) != 0) {
			return -1;
		}
	}
	
	dstr_init(&dstr, 256);
	dstr_append_zt(&dstr, "Subscription-State: ");
	switch (s->subscription.status) {
		case subscription_active: 
				dstr_append_zt(&dstr, "active\r\n");
				break;
		case subscription_pending: 
				dstr_append_zt(&dstr, "pending\r\n");
				break;
		case subscription_terminated_pending: 
		case subscription_terminated: 
				dstr_append_zt(&dstr, "terminated\r\n");
				break;
		case subscription_terminated_pending_to: 
		case subscription_terminated_to: 
				dstr_append_zt(&dstr, 
					"terminated;reason=timeout\r\n");
				break;
		case subscription_uninitialized: 
				dstr_append_zt(&dstr, "pending\r\n");
				/* this is an error ! */
				LOG(L_ERR, "sending NOTIFY for an unitialized subscription!\n");
				break;
	}
	dstr_append_str(&dstr, &s->subscription.contact);
	
	dstr_append_zt(&dstr, "Event: ");
	dstr_append_str(&dstr, rls_get_package(s));
	dstr_append_zt(&dstr, "\r\n");
	dstr_append_zt(&dstr, "Require: eventlist\r\n");
	dstr_append_str(&dstr, &content_type);

	headers.len = dstr_get_data_length(&dstr);
	headers.s = pkg_malloc(headers.len);
	if (!headers.s) headers.len = 0;
	else dstr_get_data(&dstr, headers.s);
	dstr_destroy(&dstr);

	TRACE_LOG("sending NOTIFY message to %.*s (subscription %p)\n", 
			dlg->rem_uri.len, 
			ZSW(dlg->rem_uri.s), s);
	
	if (sm_subscription_terminated(&s->subscription) == 0) {
		/* doesn't matter if delivered or not, it will be freed otherwise !!! */
		res = tmb.t_request_within(&method, &headers, &doc, dlg, 0, 0);
	}
	else {
		/* the subscritpion will be destroyed if NOTIFY delivery problems */
		rls_unlock(); /* the callback locks this mutex ! */
		
		/* !!!! FIXME: callbacks can't be safely used (may be called or not,
		 * may free memory automaticaly or not) !!! */
		res = tmb.t_request_within(&method, &headers, &doc, dlg, 0, 0);
		
/*		res = tmb.t_request_within(&method, &headers, &doc, dlg, rls_notify_cb, s); */
		rls_lock(); /* the callback locks this mutex ! */
		if (res < 0) {
			/* does this mean, that the callback was not called ??? */
			LOG(L_ERR, "rls_generate_notify(): t_request_within FAILED: %d! Freeing RL subscription.\n", res);
			rls_free(s); /* ?????? */
		}
	}
	
	if (doc.s) pkg_free(doc.s);
	if (content_type.s) pkg_free(content_type.s);
	if (headers.s) pkg_free(headers.s);
	return res;
}
	
int rls_prepare_subscription_response(rl_subscription_t *s, struct sip_msg *m) {
	/* char *hdr = "Supported: eventlist\r\n"; */
	char *hdr = "Require: eventlist\r\n";
	if (!add_lump_rpl(m, hdr, strlen(hdr), LUMP_RPL_HDR)) return -1;
	
	return sm_prepare_subscription_response(rls_manager, &s->subscription, m);
}

