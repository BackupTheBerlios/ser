#include "euac_funcs.h"
#include "euac_internals.h"
#include "euac_state_machine.h"
#include "../../parser/parse_expires.h"
#include "../../modules/tm/ut.h"
#include "../../parser/contact/parse_contact.h"

#include <presence/notifier.h>

int get_expiration_value(struct sip_msg *m)
{
	exp_body_t *expires = NULL;
	int e = 0;

	if (parse_headers(m, HDR_EXPIRES_F, 0) == -1) {
		/* can't parse expires header */
		return 0;
	}
	if (m->expires) {
		if (parse_expires(m->expires) < 0) {
			return 0;
		}

		expires = (exp_body_t *)m->expires->parsed;
		if (expires) if (expires->valid) e = expires->val;
	}
	/* ERR("subscription will expire in %d secs\n", e); */
	return e;
}

void extract_contact(struct sip_msg *m, str *dst)
{
	contact_t *sip_contact = NULL;
	
	str_clear(dst);
	
	contact_iterator(&sip_contact, m, NULL);
	if (sip_contact) str_dup(dst, &sip_contact->uri);
	/* FIXME: and what the other contacts? */
}
				
int is_terminating_notify(struct sip_msg *m)
{
	int res = 0;
	struct hdr_field *h;
	static str ss = STR_STATIC_INIT("Subscription-State");
	static str terminated = STR_STATIC_INIT("terminated");

	if (parse_headers(m, HDR_EOH_F, 0) == -1) {
		ERR("can't parse NOTIFY message\n");
		return 0;
	}
	h = m->headers;
	while (h) {
		/* try to find Subscription-Status with "terminated" */
		if (str_nocase_equals(&h->name, &ss) == 0) {
			if (str_str(&h->body, &terminated)) return 1;
			else return 0;
		}
		h = h->next;
	}

	return res;
}

events_uac_t *find_euac_nolock(struct sip_msg *m)
{
	dlg_id_t id;
	events_uac_t *uac;

	if (parse_headers(m, HDR_FROM_F | HDR_TO_F | HDR_CALLID_F, 0) < 0) {
		ERR("can't parse headers\n");
		return NULL;
	}
	
	memset(&id, 0, sizeof(id));
	if (m->to) id.loc_tag = ((struct to_body*)m->to->parsed)->tag_value;
	if (m->from) id.rem_tag = ((struct to_body*)m->from->parsed)->tag_value;
	if (m->callid) id.call_id = m->callid->body;

	uac = (events_uac_t*)ht_find(&euac_internals->ht_confirmed, &id);
	if (!uac) {
/*		INFO("confirmed dialog not found for arriving NOTIFY: "
			"%.*s * %.*s * %.*s\n",
			FMT_STR(id.loc_tag),
			FMT_STR(id.rem_tag),
			FMT_STR(id.call_id));*/

		id.rem_tag.len = 0;
		
		uac = (events_uac_t*)ht_find(&euac_internals->ht_unconfirmed, &id);
/*		
		if (!uac) {
			WARN("events UAC not found for arriving NOTIFY: "
				"%.*s * %.*s * %.*s\n",
				FMT_STR(id.loc_tag),
				FMT_STR(id.rem_tag),
				FMT_STR(id.call_id));
		}
		else INFO("received NOTIFY for unconfirmed dialog!\n");*/
	}

	return uac;
}

				
int remove_euac_reference_nolock(events_uac_t *uac)
{
	/* must be called from locked section !! */

	if (remove_reference(&uac->ref_cntr)) {
		/* all other references are freed - we can remove uac from the list */
		if (uac->status == euac_destroyed) {
			/* TRACE("freeing uac %p\n", uac); */
		}
		else {
			ERR("BUG: freeing uac %p in incorrect status (%d)\n", uac, uac->status);
		}
		remove_uac_from_list(uac);
		free_events_uac(uac);
		return 1;
	}
	return 0;
}

/* void rls_notify_cb(struct cell* t, struct sip_msg* msg, int code, void *param) */
static void subscribe_cb(struct cell* t, int type, struct tmcb_params* params)
{
	events_uac_t *uac = NULL;
	euac_action_t action;

	if (!params) return;

	/* FIXME: Problems
	 * 1. sometimes no response arrives (neither generated 408)
	 * 2. sometimes are more responses going here !!!
	 *  => try to find uac, if exists then process the response
	 *  otherwise ignore it
	 */

	if (params->param) uac = (events_uac_t *)*(params->param);
	if (!uac) {
		ERR("something wrong - empty uac parameter given to callback function\n");
		return;
	}

	/* TRACE("%d response on SUBSCRIBE %p\n", params->code, uac); */

	action = act_4xx;
	if ((params->code >= 100) && (params->code < 200)) action = act_1xx;
	if ((params->code >= 200) && (params->code < 300)) action = act_2xx;
	if ((params->code >= 300) && (params->code < 400)) action = act_3xx;
	
	lock_events_uac();
	euac_do_step(action, params->rpl, uac);
	/* in euac_do_step MUST be called accept_response or decline_response */
	unlock_events_uac();
}

const char *proto2uri_param(int proto)
{
	static char *none = "";
	/* static char *udp = ";transport=udp"; */
	static char *tcp = ";transport=tcp";
	static char *tls = ";transport=tls";
	static char *sctp = ";transport=sctp";
	
	switch (proto) {
		case PROTO_NONE:
		case PROTO_UDP: return none;
		case PROTO_TCP: return tcp;
		case PROTO_TLS: return tls;
		case PROTO_SCTP: return sctp;
	}
	return none;
}

/* returns length of added string */
static int get_contact_hdr(char *dst, int max_size, dlg_t *dialog)
{
	struct socket_info *send_sock;
	union sockaddr_union to_su;
	int port = 5060;
	const char *proto;
	int len;
	
	send_sock = uri2sock(0, dialog->hooks.next_hop, 
			&to_su, PROTO_NONE);
	if (!send_sock) { /* error */
		return 0;
	}
	/* send_sock = get_send_socket(NULL, to, proto); */

	proto = proto2uri_param(send_sock->proto);
	if (send_sock->port_no) port = send_sock->port_no;
	len = snprintf(dst, max_size, "Contact: <sip:%.*s:%d%s>\r\n", 
			FMT_STR(send_sock->address_str), port, proto);

	/* TRACE("%.*s (len = %d)\n", len, dst, len); */
	return len;
}

static int prepare_hdrs(events_uac_t *uac, str *hdr, str *contact_to_send)
{
	char tmp[256];
	str tmps;
	int res, contact_len;
	str warning = STR_STATIC_INIT("P-Hint: trying new subscription after 3xx\r\n");
	
	str_clear(hdr);
	res = 0;

	tmps.len = sprintf(tmp, "Expires: %d\r\n", subscribe_time);
	tmps.s = tmp;
	contact_len = get_contact_hdr(tmps.s + tmps.len, 
			sizeof(tmp) - tmps.len, uac->dialog);
	
	if (contact_len <= 0) {
		ERR("BUG: can't send SUBSCRIBE without contact\n");
		res = -1;
	}
	
	if (res == 0) {
		tmps.len += contact_len;
		res = str_concat(hdr, &uac->headers, &tmps);
	}

	if (!is_str_empty(contact_to_send)) {
		/* FIXME - only testing */
		str s = *hdr;
		if (res == 0) res = str_concat(hdr, &s, &warning);
	}
	if (res != 0) {
		str_free_content(hdr);
	}
	return res;
}

int new_subscription(events_uac_t *uac, str *contact_to_send, int failover_time)
{
	static str method = STR_STATIC_INIT("SUBSCRIBE");
	unsigned int cseq = 1;
	str hdr = STR_NULL;
	str *uri;
	
	if (!is_str_empty(contact_to_send)) uri = contact_to_send;
	else uri = &uac->remote_uri;
		
	/* create new dialog */
	if (euac_internals->tmb.new_dlg_uac(NULL /* will be generated */,
				NULL /* will be generated */, 
				cseq, &uac->local_uri, 
				uri, &uac->dialog) < 0) {
		ERR("can't create dialog for URI \'%.*s\'\n", FMT_STR(uac->remote_uri));
		goto ns_err_nodlg;
	}
	
	/* preset route for created dialog */
	if (!is_str_empty(&uac->route)) 
		if (euac_internals->dlgb.preset_dialog_route(uac->dialog, &uac->route) < 0)
			goto ns_err_dlg;

	if (prepare_hdrs(uac, &hdr, contact_to_send) < 0) goto ns_err_dlg;

	add_reference(&uac->ref_cntr); /* add reference for callback function */

	/* add to hash table (hash acording to dialog id) */
	DBG("adding into unconfirmed EUACs\n");
	if (ht_add(&euac_internals->ht_unconfirmed, &uac->dialog->id, uac) != 0) 
		goto ns_err_ref;
	
	/* ERR("DIALOG ID = %.*s:%.*s:%.*s\n",
			FMT_STR(uac->dialog->id.call_id), 
			FMT_STR(uac->dialog->id.rem_tag), 
			FMT_STR(uac->dialog->id.loc_tag)); */

	/* generate subscribe request */
	if (euac_internals->dlgb.request_outside(&method, 
			&hdr, NULL /* no body */, 
			uac->dialog, subscribe_cb, uac) < 0)
		goto ns_err_in_ht;

	str_free_content(&hdr);

	if (failover_time > 0) euac_set_timer(uac, failover_time);
	
	return 0;
	
ns_err_in_ht:
	ht_remove(&euac_internals->ht_unconfirmed, &uac->dialog->id);
	
ns_err_ref:
	remove_reference(&uac->ref_cntr);

ns_err_dlg:
	if (uac->dialog) euac_internals->tmb.free_dlg(uac->dialog);
	
ns_err_nodlg:
	uac->dialog = NULL;
/*	euac_do_step(act_4xx, NULL, uac); */
	str_free_content(&hdr);
	
	return -1;
}

int renew_subscription(events_uac_t *uac, int expires, int failover_time)
{
	static str method = STR_STATIC_INIT("SUBSCRIBE");
	int res, contact_len;
	str hdr;
	char tmp[256];
	str tmps;

	tmps.len = sprintf(tmp, "Expires: %d\r\n", expires);
	tmps.s = tmp;
	contact_len = get_contact_hdr(tmps.s + tmps.len, 
			sizeof(tmp) - tmps.len, uac->dialog);
	if (contact_len <= 0) {
		ERR("BUG: can't send SUBSCRIBE without contact\n");
/*		euac_do_step(act_4xx, NULL, uac); */
		return -1;
	}
	tmps.len += contact_len;
	if (str_concat(&hdr, &uac->headers, &tmps) < 0) {
		ERR("can't build headers\n");
		/* euac_do_step(act_4xx, NULL, uac); */
		return -1;
	}

	/* TRACE("sending resubscribe with hdrs: %.*s\n", FMT_STR(hdr)); */

	/* generate subscribe request */
	add_reference(&uac->ref_cntr); /* add reference for callback function */

	/* generate subscribe request - don't call the TM version
	 * (frees callback params on error!!!) */
	res = euac_internals->dlgb.request_inside(&method, 
			&hdr, NULL /* no body */, 
			uac->dialog, subscribe_cb, uac);

	str_free_content(&hdr);
	
	if (res < 0) {
/*		euac_do_step(act_4xx, NULL, uac); */
		remove_reference(&uac->ref_cntr); /* remove reference for cb function */
		return res;
	}
	else {
		if (failover_time > 0) euac_set_timer(uac, failover_time);
		return 0;
	}
}

void do_notification(events_uac_t *uac, struct sip_msg *m)
{
	DBG("received notification\n");

	if (!m) return;
	
	if (euac_internals->tmb.t_reply(m, 200, "OK") == -1) {
		ERR("Error while sending response!\n");
	}

	if (uac) {
		if (uac->cb) uac->cb(m, uac->cbp);
	}
}

void discard_notification(events_uac_t *uac, struct sip_msg *m, int res_code, char *msg)
{
	/* might be called on destroyed dialog !*/
	DBG("received notification (discard)\n");
	
	if (!m) return;
	
	if (euac_internals->tmb.t_reply(m, res_code, msg) == -1) {
		ERR("Error while sending response: %d %s\n", res_code, msg);
	}
}

void refresh_dialog(events_uac_t *uac, struct sip_msg *m)
{
	if (uac->dialog)
		euac_internals->tmb.dlg_request_uas(uac->dialog, m);
}

void refresh_dialog_resp(events_uac_t *uac, struct sip_msg *m)
{
	if (uac->dialog)
		euac_internals->tmb.dlg_response_uac(uac->dialog, m);
}

/* ----- Timer functions ----- */

static ticks_t timer_cb(ticks_t ticks, struct timer_ln* tl, void* data)
{
	events_uac_t *uac = (events_uac_t*)data;

	if (!uac) {
		ERR("BUG: null parameter\n");
		return 0;
	}

	/* TRACE("timer called at %d ticks with %p\n", ticks, data); */

	lock_events_uac();
	uac->timer_started = 0;
	euac_do_step(act_tick, NULL, uac);
	remove_euac_reference_nolock(uac);
	unlock_events_uac();
	return 0; /* one shot timer */
}

void euac_set_timer(events_uac_t *uac, int seconds)
{
	/* set timer to generate act_tick action after "seconds" seconds */
	if (uac->timer_started) euac_clear_timer(uac);
	
	add_reference(&uac->ref_cntr); /* add reference for timer callback function */
	timer_init(&uac->timer, timer_cb, uac, 0);
	if (timer_add(&uac->timer, S_TO_TICKS(seconds)) != 0) {
		ERR("can't set timer for [%s]!\n", uac->id);
	}
	uac->timer_started = 1;
	/* TRACE("timer added for %d secs\n", seconds); */
}

void euac_clear_timer(events_uac_t *uac)
{
	/* unset timer */
	/* timer->data = NULL; */
	/* TRACE("clearing timer\n"); */
	if (uac->timer_started) {
		uac->timer_started = 0;
		timer_del(&uac->timer);
		remove_euac_reference_nolock(uac);
	}
}
