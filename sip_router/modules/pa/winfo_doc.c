#include "winfo_doc.h"
#include <cds/dstring.h>
#include <cds/sstr.h>

static int doc_add_watcher(dstring_t *buf, watcher_t *w)
{
	char tmp[64];
	
	dstr_append_zt(buf, "\t\t<watcher status=\"");
	dstr_append_str(buf, &watcher_status_names[w->status]);
	dstr_append_zt(buf, "\" event=\"");
	dstr_append_str(buf, &watcher_event_names[w->event]);
	dstr_append_zt(buf, "\" id=\"");

	if (w->s_id.len < 1) {
		sprintf(tmp, "%p", w);
		dstr_append_zt(buf, tmp);
	}
	else dstr_append_str(buf, &w->s_id);
	dstr_append_zt(buf, "\">");
	
	dstr_append_str(buf, &w->uri);

	dstr_append_zt(buf, "</watcher>\r\n");
	
	return 0;
}

static int doc_add_internal_watcher(dstring_t *buf, internal_pa_subscription_t *iw)
{
	char tmp[64];
	
	dstr_append_zt(buf, "\t\t<watcher status=\"");
	dstr_append_str(buf, &watcher_status_names[iw->status]);
	dstr_append_zt(buf, "\" event=\"");
	dstr_append_str(buf, &watcher_event_names[0]);
	dstr_append_zt(buf, "\" id=\"");

	sprintf(tmp, "%pi", iw);
	dstr_append_zt(buf, tmp);
	dstr_append_zt(buf, "\">");
	
	dstr_append_str(buf, &iw->subscription->subscriber_id);

	dstr_append_zt(buf, "</watcher>\r\n");
	return 0;
}

static int doc_add_watcher_list(dstring_t *buf, struct presentity* p)
{
	watcher_t *watcher = p->watchers;
	internal_pa_subscription_t *subscription = p->first_qsa_subscription;
	
	dstr_append_zt(buf, "\t<watcher-list resource=\"");
	dstr_append_str(buf, &p->uri);
	dstr_append_zt(buf, "\" package=\"presence\">\r\n");

	while (watcher) {
		doc_add_watcher(buf, watcher);
		watcher = watcher->next;
	}

	while (subscription) {
		doc_add_internal_watcher(buf, subscription);
		subscription = subscription->next;
	}

	dstr_append_zt(buf, "\t</watcher-list>\r\n");
	return 0;
}
	
static int doc_add_winfo(dstring_t *buf, struct presentity* p, struct watcher* w)
{
	char tmp[256];
	
	dstr_append_zt(buf, "<watcherinfo xmlns=\"urn:ietf:params:xml:ns:watcherinfo\" version=\"");
	sprintf(tmp, "%d", w->document_index);
	dstr_append_zt(buf, tmp);
	dstr_append_zt(buf, "\" state=\"full\">\r\n");

	doc_add_watcher_list(buf, p);
	
	dstr_append_zt(buf, "</watcherinfo>\r\n");
	return 0;
}

int create_winfo_document(struct presentity* p, struct watcher* w, str *dst, str *dst_content_type)
{
	dstring_t buf;
	
	if (!dst) return -1;
	
	str_clear(dst);
	if (dst_content_type) str_clear(dst_content_type);

	if ((!p) || (!w))  return -1;
	
	if (dst_content_type) 
		str_dup_zt(dst_content_type, "application/watcherinfo+xml");

/*	if (!p->first_tuple) return 0;*/	/* no tuples => nothing to say */ 
	
	dstr_init(&buf, 2048);
	
	dstr_append_zt(&buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n");
	doc_add_winfo(&buf, p, w);
	
	dst->len = dstr_get_data_length(&buf);
	dst->s = cds_malloc(dst->len);
	if (!dst->s) dst->len = 0;
	else dstr_get_data(&buf, dst->s);
	dstr_destroy(&buf);
	
	return 0;
}

static int doc_add_watcher_offline(dstring_t *buf, offline_winfo_t *w)
{
	char tmp[64];
	char *status = "unknown";
	char *wevent = "subscribe";
	
	dstr_append_zt(buf, "\t\t<watcher status=\"");
	dstr_append_zt(buf, status);
	dstr_append_zt(buf, "\" event=\"");
	dstr_append_zt(buf, wevent);
	dstr_append_zt(buf, "\" id=\"");

	sprintf(tmp, "ol%p%x", w, rand());
	dstr_append_zt(buf, tmp);
	dstr_append_zt(buf, "\">");
	
	dstr_append_str(buf, &w->watcher);
	dstr_append_zt(buf, "</watcher>\r\n");
	
	return 0;
}

static int doc_add_watcher_list_offline(dstring_t *buf, struct presentity* p, offline_winfo_t *info)
{
	offline_winfo_t *i = info;
	dstr_append_zt(buf, "\t<watcher-list resource=\"");
	dstr_append_str(buf, &p->uri);
	dstr_append_zt(buf, "\" package=\"presence\">\r\n");

	while (i) {
		doc_add_watcher_offline(buf, i);
		i = i->next;
	}

	dstr_append_zt(buf, "\t</watcher-list>\r\n");
	return 0;
}

static int doc_add_winfo_offline(dstring_t *buf, struct presentity* p, struct watcher* w, offline_winfo_t *info)
{
	char tmp[256];
	
	dstr_append_zt(buf, "<watcherinfo xmlns=\"urn:ietf:params:xml:ns:watcherinfo\" version=\"");
	sprintf(tmp, "%d", w->document_index);
	dstr_append_zt(buf, tmp);
	dstr_append_zt(buf, "\" state=\"partial\">\r\n"); /* !!! only partial notification !!! */

	doc_add_watcher_list_offline(buf, p, info);
	
	dstr_append_zt(buf, "</watcherinfo>\r\n");
	return 0;
}

/* create the NOTIFY from given infos */
int create_winfo_document_offline(struct presentity* p, struct watcher* w, 
		offline_winfo_t *infos, str *dst, str *dst_content_type)
{
	dstring_t buf;
	
	if (!dst) return -1;
	
	str_clear(dst);
	if (dst_content_type) str_clear(dst_content_type);

	if ((!p) || (!w))  return -1;
	
	if (dst_content_type) 
		str_dup_zt(dst_content_type, "application/watcherinfo+xml");

/*	if (!p->first_tuple) return 0;*/	/* no tuples => nothing to say */ 
	
	dstr_init(&buf, 2048);
	
	dstr_append_zt(&buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n");
	doc_add_winfo_offline(&buf, p, w, infos);
	
	dst->len = dstr_get_data_length(&buf);
	dst->s = cds_malloc(dst->len);
	if (!dst->s) dst->len = 0;
	else dstr_get_data(&buf, dst->s);
	dstr_destroy(&buf);
	
	return 0;
}
