#include "../../sr_module.h"
#include "../../mem/mem.h"
#include "../../mem/shm_mem.h"

#include <libxml/parser.h>
#include <curl/curl.h>
#include <cds/memory.h>
#include <cds/logger.h>
#include <cds/cds.h>
#include <xcap/xcap_client.h>
#include <cds/sstr.h>
#include <cds/dstring.h>

#include "xcap_params.h"

MODULE_VERSION

int xcap_mod_init(void);
void xcap_mod_destroy(void);
int xcap_child_init(int _rank);


int xcap_query_impl(const char *uri, xcap_query_params_t *params, char **buf, int *bsize);

/** Exported functions */
static cmd_export_t cmds[]={
	{"xcap_query", (cmd_function)xcap_query_impl, 0, 0, -1}, 
	{"fill_xcap_params", (cmd_function)fill_xcap_params_impl, 0, 0, -1}, 

	{"set_xcap_root", set_xcap_root, 1, 0, REQUEST_ROUTE | FAILURE_ROUTE},
	{"set_xcap_filename", set_xcap_filename, 1, 0, REQUEST_ROUTE | FAILURE_ROUTE},
	{0, 0, 0, 0, 0}
};

/** Exported parameters */
static param_export_t params[]={
	{"xcap_root", PARAM_STR, &default_xcap_root },
/*	{"wait_for_term_notify", PARAM_INT, &waiting_for_notify_time }, */
	{0, 0, 0}
};

struct module_exports exports = {
	"xcap",
	cmds,        /* Exported functions */
	0,           /* RPC methods */
	params,      /* Exported parameters */
	xcap_mod_init, /* module initialization function */
	0,           /* response function*/
	xcap_mod_destroy,	/* pa_destroy,  / * destroy function */
	0,           /* oncancel function */
	xcap_child_init	/* per-child init function */
};

int xcap_mod_init(void)
{
	INFO("xcap module initialization\n");

	/* ??? if other module uses this libraries it might be a problem ??? */
	DEBUG_LOG(" ... libxml\n");
	xmlInitParser();
	DEBUG_LOG(" ... libcurl\n");
	curl_global_init(CURL_GLOBAL_ALL);

	DEBUG_LOG(" ... common libraries\n");
	cds_initialize();

	return 0;
}

int xcap_child_init(int _rank)
{
	return 0;
}

void xcap_mod_destroy(void)
{
	/*int i, cnt;
	char *s;*/

	DEBUG_LOG("xcap module cleanup\n");

	DEBUG_LOG(" ... common libs\n");
	cds_cleanup();

	/* ??? if other module uses this libraries it might be a problem ??? */
/*	xmlCleanupParser();
	curl_global_cleanup();*/
	DEBUG_LOG("xcap module cleanup finished\n");
}

/* --------------------------------------------------------- */

static size_t write_data_func(void *ptr, size_t size, size_t nmemb, void *stream)
{
	int s = size * nmemb;
/*	TRACE_LOG("%d bytes writen\n", s);*/
	if (s != 0) {
		if (dstr_append((dstring_t*)stream, ptr, s) != 0) {
			ERROR_LOG("can't append %d bytes into data buffer\n", s);
			return 0;
		}
	}
	return s;
}

/* helper functions for XCAP queries */

int xcap_query_impl(const char *uri, xcap_query_params_t *params, char **buf, int *bsize)
{
	CURLcode res = -1;
	static CURL *handle = NULL;
	dstring_t data;
	char *auth = NULL;
	int i;
	long auth_methods;
	
	if (!uri) return -1;
	if (!buf) return -1;

	i = 0;
	if (params) {
		if (params->auth_user) i += strlen(params->auth_user);
		if (params->auth_pass) i += strlen(params->auth_pass);
	}
	if (i > 0) {
		/* do authentication */
		auth = (char *)cds_malloc(i + 2);
		if (!auth) return -1;
		sprintf(auth, "%s:%s", params->auth_user ? params->auth_user: "",
				params->auth_pass ? params->auth_pass: "");
	}

	auth_methods = CURLAUTH_BASIC | CURLAUTH_DIGEST;
	
	dstr_init(&data, 512);
	
	if (!handle) handle = curl_easy_init(); 
	if (handle) {
		curl_easy_setopt(handle, CURLOPT_URL, uri);
		/* TRACE_LOG("uri: %s\n", uri ? uri : "<null>"); */
		
		/* do not store data into a file - store them in memory */
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data_func);
		curl_easy_setopt(handle, CURLOPT_WRITEDATA, &data);

		/* be quiet */
		curl_easy_setopt(handle, CURLOPT_MUTE, 1);
		
		/* non-2xx => error */
		curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1);

		/* auth */
		curl_easy_setopt(handle, CURLOPT_HTTPAUTH, auth_methods); /* TODO possibility of selection */
		curl_easy_setopt(handle, CURLOPT_NETRC, CURL_NETRC_IGNORED);
		curl_easy_setopt(handle, CURLOPT_USERPWD, auth);

		/* SSL */
		if (params) {
			if (params->enable_unverified_ssl_peer) {
				curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
				curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0);
			}
		}
		
		/* follow redirects (needed for apache mod_speling - case insesitive names) */
		curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
		
	/*	curl_easy_setopt(handle, CURLOPT_TCP_NODELAY, 1);
		curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 10);*/
		
		/* Accept headers */
		
		res = curl_easy_perform(handle);
		/* curl_easy_cleanup(handle); */ /* FIXME: experimental */
	}
	else ERROR_LOG("can't initialize curl handle\n");
	if (res == 0) {
		*bsize = dstr_get_data_length(&data);
		if (*bsize) {
			*buf = (char*)cds_malloc(*bsize);
			if (!*buf) {
				ERROR_LOG("can't allocate %d bytes\n", *bsize);
				res = -1;
				*bsize = 0;
			}
			else dstr_get_data(&data, *buf);
		}
	}
	else DEBUG_LOG("curl error: %d\n", res);
	dstr_destroy(&data);
	if (auth) cds_free(auth);
	return res;
}
