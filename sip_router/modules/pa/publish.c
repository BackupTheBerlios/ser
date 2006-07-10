/*
 * Presence Agent, publish handling
 *
 * $Id: publish.c,v 1.41 2006/07/10 12:39:17 kubartv Exp $
 *
 * Copyright (C) 2001-2003 FhG Fokus
 * Copyright (C) 2003-2004 Hewlett-Packard Company
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
 *
 */

#include <string.h>
#include <stdlib.h>
#include "../../str.h"
#include "../../dprint.h"
#include "../../mem/mem.h"
#include "../../parser/parse_uri.h"
#include "../../parser/parse_from.h"
#include "../../parser/contact/parse_contact.h"
#include "../../parser/parse_expires.h"
#include "../../parser/parse_event.h"
#include "dlist.h"
#include "presentity.h"
#include "watcher.h"
#include "pstate.h"
#include "notify.h"
#include "paerrno.h"
#include "pdomain.h"
#include "pa_mod.h"
#include "ptime.h"
#include "reply.h"
#include "subscribe.h"
#include "publish.h"
#include "common.h"
#include "../../data_lump_rpl.h"
#include "../../parser/parse_sipifmatch.h"

#include <libxml/parser.h>
#include <libxml/xpath.h>

#include <presence/pidf.h>
#include <cds/logger.h>

/*
 * Parse all header fields that will be needed
 * to handle a PUBLISH request
 */
static int parse_publish_hfs(struct sip_msg* _m)
{
	int rc = 0;
	if ((rc = parse_headers(_m, HDR_FROM_F | HDR_EVENT_F | 
					HDR_EXPIRES_F | HDR_SIPIFMATCH_F | 
					HDR_CONTENTTYPE_F | HDR_CONTENTLENGTH_F, 0))
	    == -1) {
		paerrno = PA_PARSE_ERR;
		LOG(L_ERR, "parse_publish_hfs(): Error while parsing headers\n");
		return -1;
	}

	if (parse_from_header(_m) < 0) {
		paerrno = PA_FROM_ERR;
		LOG(L_ERR, "parse_publish_hfs(): From malformed or missing\n");
		return -6;
	}

	if (_m->event) {
		if (parse_event(_m->event) < 0) {
			paerrno = PA_EVENT_PARSE;
			LOG(L_ERR, "parse_publish_hfs(): Error while parsing Event header field\n");
			return -8;
		}
	} else {
		paerrno = PA_EVENT_PARSE;
		LOG(L_ERR, "parse_publish_hfs(): Missing Event header field\n");
		return -7;
	}

	if (_m->expires) {
		if (parse_expires(_m->expires) < 0) {
			paerrno = PA_EXPIRES_PARSE;
			LOG(L_ERR, "parse_publish_hfs(): Error while parsing Expires header field\n");
			return -9;
		}
	}

	/* patch from PIC-SER */
	if (_m->sipifmatch) {
		if (parse_sipifmatch(_m->sipifmatch) < 0) {
			paerrno = PA_PARSE_ERR;
			LOG(L_ERR, "parse_hfs(): Error while parsing SIP-If-Match header field\n");
			return -10;
		}
	}

	if (_m->content_type) {
		if (parse_content_type_hdr(_m) < 0) {
			LOG(L_ERR, "parse_hfs(): Can't parse Content-Type\n");
			return -12;
		}
	}
	
	return 0;
}

#if 0
/*
 * If this xcap change is on a watcher list, then reread authorizations
 */
static int publish_presentity_xcap_change(struct sip_msg* _m, struct pdomain* _d, struct presentity* presentity, int *pchanged)
{
	char *body = get_body(_m);
	LOG(L_ERR, "publish_presentity_xcap_change: body=%p\n", body);
	if (body) {
		/* cheesy hack to see if it is presence-lists or watcherinfo that was changed */
		if (strstr(body, "presence-lists"))
			presentity->flags |= PFLAG_PRESENCE_LISTS_CHANGED;
		if (strstr(body, "watcherinfo"))
			presentity->flags |= PFLAG_WATCHERINFO_CHANGED;
		presentity->flags |= PFLAG_XCAP_CHANGED;

		LOG(L_ERR, "publish_presentity_xcap_change: got body, setting flags=%x", 
		    presentity->flags);

		if (pchanged)
			*pchanged = 1;
	}
	return 0;
}
#endif

static void add_expires_to_rpl(struct sip_msg *_m, int expires)
{
	char tmp[64];
	
	if (expires < 0) expires = 0;
	
	sprintf(tmp, "Expires: %d\r\n", expires);
	if (!add_lump_rpl(_m, tmp, strlen(tmp), LUMP_RPL_HDR)) {
		LOG(L_ERR, "Can't add expires header to the response\n");
	}
}

static void add_etag_to_rpl(struct sip_msg *_m, str *etag)
{
	char *tmp;

	if (!etag) {
		LOG(L_ERR, "Can't add empty SIP-ETag header to the response\n");
		return;
	}
	
	tmp = (char*)pkg_malloc(32 + etag->len);
	if (!tmp) {
		LOG(L_ERR, "Can't allocate package memory for SIP-ETag header to the response\n");
		return;
	}
	
	sprintf(tmp, "SIP-ETag: %.*s\r\n", etag->len, ZSW(etag->s));
	if (!add_lump_rpl(_m, tmp, strlen(tmp), LUMP_RPL_HDR)) {
		LOG(L_ERR, "Can't add SIP-ETag header to the response\n");
		/* return -1; */
	}
	pkg_free(tmp);
}

static void generate_etag(str *dst, presentity_t *p)
{
	if (!dst) return;
	char tmp[128];

	/* this might not be sufficient !!! */
	sprintf(tmp, "%px%xx%x", p, rand(), (unsigned int)time(NULL));
	str_dup_zt(dst, tmp);
}

static pa_presence_note_t *presence_note2pa(presence_note_t *n, str *etag, time_t expires)
{
	return create_pres_note(etag, &n->value, &n->lang, expires, NULL);
}

static void add_presentity_notes(presentity_t *presentity, presentity_info_t *p, str *etag, time_t expires)
{
	presence_note_t *n;
	pa_presence_note_t *pan;

	if (!p) return;
	
	n = p->first_note;
	while (n) {
		pan = presence_note2pa(n, etag, expires);
		if (pan) add_pres_note(presentity, pan);
		n = n->next;
	}
}

static pa_person_element_t *person_element2pa(person_t *n, str *etag, time_t expires)
{
	return create_person_element(etag, &n->person_element, &n->id, expires, NULL);
}

static void add_person_elements(presentity_t *presentity, presentity_info_t *p, str *etag, time_t expires)
{
	person_t *n;
	pa_person_element_t *pan;

	if (!p) return;
	
	n = p->first_person;
	while (n) {
		pan = person_element2pa(n, etag, expires);
		if (pan) add_person_element(presentity, pan);
		n = n->next;
	}
}

presence_tuple_t *presence_tuple_info2pa(presence_tuple_info_t *i, str *etag, time_t expires)
{
	presence_tuple_t *t = NULL;
	presence_note_t *n, *nn;
	int res;
			
	res = new_presence_tuple(&i->contact, expires, &t, 1);
	if (res != 0) return NULL;
	/* ID for the tuple is newly generated ! */
	t->priority = i->priority;
	switch (i->status) {
		case presence_tuple_open: t->state = PS_ONLINE; break;
		case presence_tuple_closed: t->state = PS_OFFLINE; break;
	}
	str_dup(&t->etag, etag);
	str_dup(&t->published_id, &i->id); /* store published tuple ID - used on update */

	/* add notes for tuple */
	n = i->first_note;
	while (n) {
		nn = create_presence_note(&n->value, &n->lang);
		if (nn) add_tuple_note_no_wb(t, nn);
		n = n->next;
	}
	return t;
}

static void update_tuple(presentity_t *p, presence_tuple_t *t, presence_tuple_info_t *i, time_t expires)
{
	presence_note_t *n, *nn;
	
	t->expires = expires;
	t->priority = i->priority;
	switch (i->status) {
		case presence_tuple_open: t->state = PS_ONLINE; break;
		case presence_tuple_closed: t->state = PS_OFFLINE; break;
	}
	/* FIXME: enable other changes like contact ??? */

	/* remove all old notes for this tuple */
	free_tuple_notes(t);
		
	/* add new notes for tuple */
	n = i->first_note;
	while (n) {
		nn = create_presence_note(&n->value, &n->lang);
		if (nn) add_tuple_note_no_wb(t, nn);
		
		n = n->next;
	}

	if (use_db) db_update_presence_tuple(p, t, 1);
}

static void add_published_tuples(presentity_t *presentity, presentity_info_t *p, str *etag, time_t expires)
{
	presence_tuple_info_t *i;
	presence_tuple_t *t;

	if (!p) return;

	i = p->first_tuple;
	while (i) {
		t = presence_tuple_info2pa(i, etag, expires);
		if (t) add_presence_tuple(presentity, t);
		i = i->next;
	}
}

static presence_tuple_t *find_published_tuple(presentity_t *presentity, str *etag, str *id)
{
	presence_tuple_t *tuple = presentity->tuples;
	while (tuple) {
		if (str_case_equals(&tuple->etag, etag) == 0) {
			if (str_case_equals(&tuple->published_id, id) == 0)
				return tuple;
		}
		tuple = tuple->next;
	}
	return NULL;
}

static int update_published_tuples(presentity_t *presentity, presentity_info_t *p, str *etag, time_t expires)
{
	presence_tuple_info_t *i;
	presence_tuple_t *t, *tt;
	int found = 0;
	double mark = -149.386;

	if (!p) return 0;
	
	/* mark tuples as unprocessed */
	t = presentity->tuples;
	while (t) {
		if (str_case_equals(&t->etag, etag) == 0) {
			t->priority = mark;
			found++;
		}
		t = t->next;
	}
	
	/* add previously not published tuples and update previously published */
	i = p->first_tuple;
	while (i) {
		t = find_published_tuple(presentity, etag, &i->id);
		if (t) {
			/* the tuple was published this way */
			found++;
			update_tuple(presentity, t, i, expires);
		}
		else {
			/* this tuple was not published => add it */
			t = presence_tuple_info2pa(i, etag, expires);
			if (t) add_presence_tuple(presentity, t);
		}
		i = i->next;
	}
	
	/* remove previously published tuples which were not processed (not present now) */
	t = presentity->tuples;
	while (t) {
		tt = t->next;
		if (t->priority == mark) {
			remove_presence_tuple(presentity, t);
			free_presence_tuple(t);
		}
		t = tt;
	}

	return found;
}

static int update_all_published_tuples(presentity_t *p, str *etag, time_t expires)
{
	int found = 0;
	presence_tuple_t *tuple = p->tuples;
	while (tuple) {
		if (str_case_equals(&tuple->etag, etag) == 0) {
			tuple->expires = expires;
			found++;
			db_update_presence_tuple(p, tuple, 0);
		}
		tuple = tuple->next;
	}
	return found;
}

static int update_pres_notes(presentity_t *p, str *etag, time_t expires)
{
	int found = 0;
	pa_presence_note_t *note = p->notes;
	while (note) {
		if (str_case_equals(&note->etag, etag) == 0) {
			note->expires = expires;
			found++;
			db_update_pres_note(p, note);
		}
		note = note->next;
	}
	return found;
}

static int update_person_elements(presentity_t *p, str *etag, time_t expires)
{
	int found = 0;
	pa_person_element_t *person = p->person_elements;
	while (person) {
		if (str_case_equals(&person->etag, etag) == 0) {
			person->expires = expires;
			db_update_person_element(p, person);
			found++;
		}
		person = person->next;
	}
	return found;
}

int process_published_presentity_info(presentity_t *presentity, presentity_info_t *p, str *etag, time_t expires)
{
	if (etag->len < 1) {
		
		if (!p) return -1; /* must be published something */
		
		/* etag is empty -> generate new one and generate published info as new */
		generate_etag(etag, presentity);
		
		/* add all notes for presentity */
		add_presentity_notes(presentity, p, etag, expires);
		
		/* add all tuples */
		add_published_tuples(presentity, p, etag, expires);

		/* add all person elements (RPID) */
		add_person_elements(presentity, p, etag, expires);
	}
	else {
		if (p) {
			/* remove all notes for this etag */
			remove_pres_notes(presentity, etag);
			
			/* remove all person elements (RPID) */
			remove_person_elements(presentity, etag);
		
			/* add all notes for presentity */
			add_presentity_notes(presentity, p, etag, expires);
			update_published_tuples(presentity, p, etag, expires);
			add_person_elements(presentity, p, etag, expires);
		}
		else {
			/* all expirations must be refreshed, nothing cleared */
			update_all_published_tuples(presentity, etag, expires);
			update_person_elements(presentity, etag, expires);
			update_pres_notes(presentity, etag, expires);
		}
	}
	presentity->flags |= PFLAG_PRESENCE_CHANGED;
	return 0;
}

static int publish_presence(struct sip_msg* _m, struct presentity* presentity)
{
	char *body = get_body(_m);
	int body_len = 0;
	int msg_expires = default_expires;
	time_t expires = 0;
	str etag;
	presentity_info_t *p = NULL;
	int content_type = -1;
	
	if (_m->content_type) content_type = get_content_type(_m);
	if (_m->content_length) body_len = get_content_length(_m);
	
	if (_m->expires) {
		if (_m->expires->parsed) {
			msg_expires = ((exp_body_t*)_m->expires->parsed)->val;
		}
	}
	if (msg_expires > max_publish_expiration) 
		msg_expires = max_publish_expiration;
	if (msg_expires != 0) expires = msg_expires + act_time;

	if (_m->sipifmatch) str_dup(&etag, (str*)_m->sipifmatch->parsed);
	else str_clear(&etag);
				
	if (body_len > 0) {
		switch (content_type) {
			case MIMETYPE(APPLICATION,PIDFXML):
				if (parse_pidf_document(&p, body, body_len) != 0) {
					LOG(L_ERR, "can't parse PIDF document\n");
					paerrno = PA_UNSUPP_DOC; /* ? PA_PARSE_ERR */
				}
				break;
			case MIMETYPE(APPLICATION,CPIM_PIDFXML):
				if (parse_cpim_pidf_document(&p, body, body_len) != 0) {
					LOG(L_ERR, "can't parse CPIM-PIDF document\n");
					paerrno = PA_UNSUPP_DOC;
				}
				break;
			default:
				LOG(L_ERR, "unsupported Content-Type \'%.*s\' (0x%x) for PUBLISH handling\n", 
						FMT_STR(_m->content_type->body), content_type);
				paerrno = PA_UNSUPP_DOC;
		}
		
		if (paerrno != PA_OK) {
			str_free_content(&etag);
			return -1;
		}
	}
	
	if (process_published_presentity_info(presentity, p, &etag, expires) == 0) {
		/* add header fields into response */
		add_expires_to_rpl(_m, msg_expires);
		add_etag_to_rpl(_m, &etag);
	}
	str_free_content(&etag);
	if (p) free_presentity_info(p);
	
	return 0;
}

static int publish_presentity(struct sip_msg* _m, struct pdomain* _d, struct presentity* presentity)
{
	event_t *parsed_event = NULL;
	int event_package = EVENT_OTHER;
	str callid = STR_STATIC_INIT("???");
	int res;
	
	if (_m->event) 
		parsed_event = (event_t *)_m->event->parsed;
	if (parsed_event)
		event_package = parsed_event->parsed;

	LOG(L_DBG, "publish_presentity: event_package=%d -1-\n", event_package);
	switch (event_package) {
		case EVENT_PRESENCE: 
			res = publish_presence(_m, presentity);
			break;
		default:
			if (_m->callid)	callid = _m->callid->body;
			LOG(L_WARN, "publish_presentity: no handler for event_package=%d"
					" callid=%.*s\n", event_package, callid.len, ZSW(callid.s));
			paerrno = PA_EVENT_UNSUPP;
			res = -1;
	}

	return res;
}

/*
 * Handle a publish Request
 */

int handle_publish(struct sip_msg* _m, char* _domain, char* _s2)
{
	struct pdomain* d;
	struct presentity *p;
	str p_uri = STR_NULL;
	str uid = STR_NULL;
	xcap_query_params_t xcap_params;

	get_act_time();
	paerrno = PA_OK;

	if (parse_publish_hfs(_m) < 0) {
		LOG(L_ERR, "handle_publish(): Error while parsing message header\n");
		goto error;
	}

#if 0
	if (check_message(_m) < 0) {
		LOG(L_ERR, "handle_publish(): Error while checking message\n");
		goto error;
	}
	LOG(L_ERR, "handle_publish -1c-\n");
#endif
	LOG(L_DBG, "handle_publish entered\n");

	d = (struct pdomain*)_domain;

	if (get_pres_uri(_m, &p_uri) < 0 || p_uri.s == NULL || p_uri.len == 0) {
		LOG(L_ERR, "handle_publish(): Error while extracting presentity URI\n");
		goto error;
	}

	if (get_presentity_uid(&uid, _m) != 0) {
		LOG(L_ERR, "handle_subscription(): Error while extracting presentity UID\n");
		goto error;
	}

	lock_pdomain(d);

	if (find_presentity_uid(d, &uid, &p) > 0) {
		memset(&xcap_params, 0, sizeof(xcap_params));
		if (fill_xcap_params) fill_xcap_params(_m, &xcap_params);
		if (new_presentity(d, &p_uri, &uid, &xcap_params, &p) < 0) {
			LOG(L_ERR, "handle_publish can't create presentity\n");
			goto error2;
		}
	}
	str_free_content(&uid);

	LOG(L_DBG, "handle_publish - publishing status\n");
	
	/* update presentity event state */
	if (p) publish_presentity(_m, d, p);

	unlock_pdomain(d);

	if (send_reply(_m) < 0) return -1;

	LOG(L_DBG, "handle_publish finished\n");
	return 1;

error2:
	unlock_pdomain(d);
	str_free_content(&uid);
error:
	send_reply(_m);
	return 0;
}
