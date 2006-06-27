/*
 * $Id: domain_mod.c,v 1.33 2006/06/27 16:12:28 janakj Exp $
 *
 * Domain module
 *
 * Copyright (C) 2002-2003 Juha Heinanen
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
 * History:
 * -------
 * 2003-03-11: New module interface (janakj)
 * 2003-03-16: flags export parameter added (janakj)
 * 2003-04-05: default_uri #define used (jiri)
 * 2003-04-06: db connection closed in mod_init (janakj)
 * 2004-06-06  updated to the new DB api, cleanup: static dbf & handler,
 *              calls to domain_db_{bind,init,close,ver} (andrei)
 */


#include "domain_mod.h"
#include <stdio.h>
#include "../../mem/shm_mem.h"
#include "../../mem/mem.h"
#include "../../sr_module.h"
#include "../../ut.h"
#include "../../parser/parse_from.h"
#include "../../parser/parse_uri.h"
#include "../../usr_avp.h"
#include "domain_rpc.h"
#include "hash.h"
#include "domain.h"


/*
 * Module management function prototypes
 */
static int mod_init(void);
static void destroy(void);
static int child_init(int rank);
static int is_from_local(struct sip_msg* msg, char* s1, char* s2);
static int is_ruri_local(struct sip_msg* msg, char* s1, char* s2);
static int is_from_anonym(struct sip_msg* msg, char* s1, char* s2);
static int lookup_domain(struct sip_msg* msg, char* s1, char* s2);
static int lookup_domain_fixup(void** param, int param_no);

MODULE_VERSION

#define LOAD_RURI 0
#define LOAD_FROM 1
#define LOAD_TO 2
#define LOAD_AVP 3

/*
 * Version of domain table required by the module,
 * increment this value if you change the table in
 * an backwards incompatible way
 */
#define DOMAIN_TABLE_VERSION 2
#define DOMATTR_TABLE_VERSION 1

#define DOMAIN_TABLE  "domain"
#define DOMAIN_COL    "domain"
#define DID_COL       "did"
#define FLAGS_COL     "flags"

#define DOMATTR_TABLE "domain_attrs"
#define DOMATTR_DID   "did"
#define DOMATTR_NAME  "name"
#define DOMATTR_TYPE  "type"
#define DOMATTR_VALUE "value"
#define DOMATTR_FLAGS "flags"
#define DOMAIN_COL    "domain"

#define PPREFEREDID     "P-Preferred-Identity"
#define ANONYM_DOMAIN   "anonymous.invalid"
#define ANONYM_FROM     "is_anonymous"
#define ANONYM_FROM_VAL "1"

int db_mode = 0;  /* Database usage mode: 0 = no cache, 1 = cache */

/*
 * Module parameter variables
 */
static str db_url = STR_STATIC_INIT(DEFAULT_RODB_URL);

str domain_table = STR_STATIC_INIT(DOMAIN_TABLE); /* Name of domain table */
str domain_col   = STR_STATIC_INIT(DOMAIN_TABLE); /* Name of domain column */
str did_col      = STR_STATIC_INIT(DID_COL);      /* Domain id */
str flags_col    = STR_STATIC_INIT(FLAGS_COL);    /* Domain flags */

str domattr_table = STR_STATIC_INIT(DOMATTR_TABLE);
str domattr_did   = STR_STATIC_INIT(DOMATTR_DID);
str domattr_name  = STR_STATIC_INIT(DOMATTR_NAME);
str domattr_type  = STR_STATIC_INIT(DOMATTR_TYPE);
str domattr_value = STR_STATIC_INIT(DOMATTR_VALUE);
str domattr_flags = STR_STATIC_INIT(DOMATTR_FLAGS);

str ppi             = STR_STATIC_INIT(PPREFEREDID);
str anonym_domain   = STR_STATIC_INIT(ANONYM_DOMAIN);
str from_anonym     = STR_STATIC_INIT(ANONYM_FROM);
str from_anonym_val = STR_STATIC_INIT(ANONYM_FROM_VAL);

int load_domain_attrs = 0;  /* Load attributes for each domain by default */

int load_prefered_id  = 0;  /* Load attributes for domain in P-Prefered-Identiy */

db_con_t* con = 0;
db_func_t db;

struct hash_entry*** active_hash = 0; /* Pointer to current hash table */
struct hash_entry** hash_1 = 0;       /* Pointer to hash table 1 */
struct hash_entry** hash_2 = 0;       /* Pointer to hash table 2 */

domain_t** domains_1 = 0;    /* List of domains 1 */
domain_t** domains_2 = 0;    /* List of domains 2 */


/*
 * Exported functions
 */
static cmd_export_t cmds[] = {
	{"is_from_local",     is_from_local,  0, 0, REQUEST_ROUTE                },
	{"is_uri_host_local", is_ruri_local,  0, 0, REQUEST_ROUTE | BRANCH_ROUTE },
	{"is_from_anonym",    is_from_anonym, 0, 0, REQUEST_ROUTE | FAILURE_ROUTE},
	{"lookup_domain",     lookup_domain,  1, lookup_domain_fixup, REQUEST_ROUTE },
	{0, 0, 0, 0, 0}
};


/*
 * Exported parameters
 */
static param_export_t params[] = {
	{"db_url",	      PARAM_STR, &db_url           },
	{"db_mode",           PARAM_INT, &db_mode          },
	{"domain_table",      PARAM_STR, &domain_table     },
	{"domain_col",        PARAM_STR, &domain_col       },
	{"did_col",           PARAM_STR, &did_col          },
	{"flags_col",         PARAM_STR, &flags_col        },
	{"domattr_table",     PARAM_STR, &domattr_table    },
	{"domattr_did",       PARAM_STR, &domattr_did      },
	{"domattr_name",      PARAM_STR, &domattr_name     },
	{"domattr_type",      PARAM_STR, &domattr_type     },
	{"domattr_value",     PARAM_STR, &domattr_value    },
	{"domattr_flags",     PARAM_STR, &domattr_flags    },
	{"load_domain_attrs", PARAM_INT, &load_domain_attrs},
	{"anonym_domain",     PARAM_STR, &anonym_domain    },
	{"load_preferred_id", PARAM_INT, &load_prefered_id },
	{0, 0, 0}
};


/*
 * Module interface
 */
struct module_exports exports = {
	"domain",
	cmds,       /* Exported functions */
	domain_rpc, /* RPC methods */
	params,     /* Exported parameters */
	mod_init,   /* module initialization function */
	0,          /* response function*/
	destroy,    /* destroy function */
	0,          /* cancel function */
	child_init  /* per-child init function */
};


static int connect_db(void)
{
	if (db.init == 0) {
		LOG(L_ERR, "BUG:domain:connect_db: No database module found\n");
		return -1;
	}

	con = db.init(db_url.s);
	if (con == 0){
		LOG(L_ERR, "ERROR:domain:connect_db: Unable to connect database %s", db_url.s);
		return -1;
	}
	return 0;
}


static void disconnect_db(void)
{
	if (con && db.close){
		db.close(con);
		con = 0;
	}
}


/*
 * Check version of domain and domain_attrs tables
 */
static int check_version(void)
{
	int ver;

	     /* Check table version */
	ver = table_version(&db, con, &domain_table);
	if (ver < 0) {
		LOG(L_ERR, "ERROR:domain:check_version: Error while querying table version\n");
		return -1;
	} else if (ver < DOMAIN_TABLE_VERSION) {
		LOG(L_ERR, "ERROR:domain:check_version: Invalid table version, update your database schema\n");
		return -1;
	}

	ver = table_version(&db, con, &domattr_table);
	if (ver < 0) {
		LOG(L_ERR, "ERROR:domain:check_version: Error while querying table version\n");
		return -1;
	} else if (ver < DOMATTR_TABLE_VERSION) {
		LOG(L_ERR, "ERROR:domain:check_version: Invalid table version, update your database schema\n");
		return -1;
	}
	return 0;
}

static int allocate_tables(void)
{

	active_hash = (struct hash_entry***)shm_malloc(sizeof(struct hash_entry**));
	hash_1 = (struct hash_entry**)shm_malloc(sizeof(struct hash_entry*) * HASH_SIZE);
	hash_2 = (struct hash_entry**)shm_malloc(sizeof(struct hash_entry*) * HASH_SIZE);
	domains_1 = (domain_t**)shm_malloc(sizeof(domain_t*));
	domains_2 = (domain_t**)shm_malloc(sizeof(domain_t*));

	if (!hash_1 || !hash_2 || !active_hash || !domains_1 || !domains_2) {
		LOG(L_ERR, "ERROR:domain:allocate_tables: No memory left\n");
		return -1;
	}
	memset(hash_1, 0, sizeof(struct hash_entry*) * HASH_SIZE);
	memset(hash_2, 0, sizeof(struct hash_entry*) * HASH_SIZE);
	*active_hash = hash_1;
	*domains_1 = 0;
	*domains_2 = 0;
	return 0;
}

static void destroy_tables(void)
{
	free_table(hash_1);
	free_table(hash_2);
	if (active_hash) shm_free(active_hash);

	if (domains_1) {
		free_domain_list(*domains_1);
		shm_free(domains_1);
	}

	if (domains_2) {
		free_domain_list(*domains_2);
		shm_free(domains_2);
	}
}

static int mod_init(void)
{
	if (load_domain_attrs && !db_mode) {
		LOG(L_ERR, "ERROR:domain:mod_init: Domain attributes only work when domain cache is enabled (set db_mode to 1)\n");
		return -1;
	}

	if (bind_dbmod(db_url.s, &db )) {
		LOG(L_CRIT, "ERROR:domain:mod_init: Cannot bind to database module! "
		    "Did you forget to load a database module ?\n");
		return -1;
	}

	     /* Check if cache needs to be loaded from domain table */
	if (db_mode != 0) {
		if (connect_db() < 0) goto error;
		if (check_version() < 0) goto error;
		if (allocate_tables() < 0) goto error;
		if (reload_domain_list() < 0) goto error;
		disconnect_db();
	}

	return 0;

 error:
	disconnect_db();
	return -1;
}


static int child_init(int rank)
{
	/* Check if database is needed by child */
	if (rank > 0 || rank == PROC_FIFO || rank == PROC_UNIXSOCK) {
		if (connect_db() < 0) return -1;
	}
	return 0;
}


static void destroy(void)
{
	/* Destroy is called from the main process only,
	 * there is no need to close database here because
	 * it is closed in mod_init already
	 */
	destroy_tables();
}


/*
 * Retrieve did directly from database, without using
 * memory cache. Use 0 as the value of first parameter
 * if you only want to know whether the entry is in the
 * database. The function returns 1 if there is such
 * entry, 0 if not, and -1 on error.
 */
static int db_get_did(str* did, str* domain)
{
	db_key_t keys[1], cols[2];
	db_val_t vals[1], *val;
	db_res_t* res;
	str t;

	keys[0]=domain_col.s;
	cols[0]=did_col.s;
	cols[1]=flags_col.s;

	if (!domain) {
		LOG(L_ERR, "BUG:domain:db_get_did: Invalid parameter value\n");
		return -1;
	}

	if (db.use_table(con, domain_table.s) < 0) {
		LOG(L_ERR, "ERROR:domain:db_get_did: Error while trying to use domain table\n");
		return -1;
	}

	vals[0].type = DB_STR;
	vals[0].nul = 0;
	vals[0].val.str_val = *domain;

	if (db.query(con, keys, 0, vals, cols, 1, 2, 0, &res) < 0) {
		LOG(L_ERR, "ERROR:domain:db_get_did: Error while querying database\n");
			return -1;
	}

	if (res->n > 0) {
		val = res->rows[0].values;

		     /* Test flags first, we are only interested in rows
		      * that are not disabled
		      */
		if (val[1].nul || (val[1].val.int_val & DB_DISABLED)) {
			db.free_result(con, res);
			return 0;
		}

		if (did) {
			if (val[0].nul) {
				did->len = 0;
			} else {
				t.s = (char*)val[0].val.string_val;
				t.len = strlen(t.s);
				if (did->len < t.len) t.len = did->len;
				memcpy(did->s, t.s, t.len);
			}
		}
		db.free_result(con, res);
		return 1;
	} else {
		db.free_result(con, res);
		return 0;
	}
}


/*
 * Check if domain is local
 */
static int is_local(str* host)
{
	if (db_mode == 0) {
		switch(db_get_did(0, host)) {
		case 1:  return 1;
		default: return -1;
		}
	} else {
		if (hash_lookup(0, *active_hash, host) == 1) return 1;
		else return -1;
	}
}


static inline int is_anonym_domain(str *dom) {
	str doml;
	int ret = 0;
	int_str avp_name, avp_val;

	if (dom->len == anonym_domain.len) {
		doml.s = pkg_malloc(dom->len);
		if (!doml.s) {
			LOG(L_ERR, "domain:is_anonym_domain: no memory left\n");
			return 0;
		}
		memcpy(doml.s, dom->s, dom->len);
		doml.len = dom->len;
		strlower(&doml);
		ret = (memcmp(doml.s, anonym_domain.s, doml.len) == 0);
		if (ret) {
			avp_name.s = from_anonym;
			avp_val.s = from_anonym_val;
			add_avp(AVP_CLASS_USER| AVP_TRACK_FROM | AVP_NAME_STR | AVP_VAL_STR, avp_name, avp_val);
		}
		pkg_free(doml.s);
	}
	return ret;
}

static int is_from_anonym(struct sip_msg* msg, char* s1, char* s2)
{
	struct sip_uri puri;
	str uri;

	if (parse_from_header(msg) < 0) {
		LOG(L_ERR, "domain:get_from_host: Error while parsing From header\n");
		return -1;
	}

	uri = get_from(msg)->uri;

	if (parse_uri(uri.s, uri.len, &puri) < 0) {
		LOG(L_ERR, "domain:get_from_host: Error while parsing From URI\n");
		return -1;
	}

	if (is_anonym_domain(&(puri.host))) {
		return 1;
	}
	else {
		return -1;
	}
}

/*
 * Extract From host and convert it to lower case, the
 * result must be disposed using pkg_free after use
 */
static int get_from_host(str* res, struct sip_msg* msg)
{
	struct sip_uri puri;
	struct sip_uri ppiuri;
	struct to_body ppibody;
	struct hdr_field *hf;
	str uri;
	
	res->s=0; /* must be initialized to 0 */
	res->len=0;

	if (parse_from_header(msg) < 0) {
		LOG(L_ERR, "domain:get_from_host: Error while parsing From header\n");
		return -2;
	}

	uri = get_from(msg)->uri;

	if (parse_uri(uri.s, uri.len, &puri) < 0) {
		LOG(L_ERR, "domain:get_from_host: Error while parsing From URI\n");
		return -3;
	}

	if (load_prefered_id &&
		is_anonym_domain(&(puri.host))) {
		/* check for a P-Preferred-Id header */
		parse_headers(msg, HDR_EOH_F, 0);
		for (hf=msg->headers; hf; hf=hf->next) {
			if (hf->name.len == ppi.len &&
					strncasecmp(hf->name.s, ppi.s, ppi.len) == 0)
				break;
		}
		if (!hf) {
			LOG(L_INFO, "domain:get_from_host: anonymous domain in From but missing '%.*s' header\n", ppi.len, ppi.s);
			goto skip_ppi;
		}
		/* we found a P-Preferred-Id header */
		memset(&ppibody, 0, sizeof(struct to_body));
		parse_to(hf->body.s, hf->body.s+hf->body.len+1, &ppibody);
		if (ppibody.error == PARSE_ERROR) {
			LOG(L_WARN, "domain:get_from_host: Failed to parse P-Preferred-Id body\n");
			goto skip_ppi;
		}
		if (parse_uri(ppibody.uri.s, ppibody.uri.len, &ppiuri) < 0) {
			LOG(L_WARN, "domain:get_from_host: Failed to parse P-Preferred-Id URI\n");
			goto skip_ppi;
		}
		res->s = pkg_malloc(ppiuri.host.len);
		if (!res->s) {
			LOG(L_ERR, "domain:get_form_host: No memory left for P-Preferred-Id URI\n");
			return -1;
		}
		LOG(L_INFO, "domain:get_from_host: using domain from '%.*s' header\n", ppi.len, ppi.s);
		memcpy(res->s, ppiuri.host.s, ppiuri.host.len);
		res->len = ppiuri.host.len;
	}
skip_ppi:
	if (!res->s) {
		res->s = pkg_malloc(puri.host.len);
		if (!res->s) {
			LOG(L_ERR, "domain:get_from_host: No memory left\n");
			return -1;
		}
		memcpy(res->s, puri.host.s, puri.host.len);
		res->len = puri.host.len;
	}
	strlower(res);
	return 0;
}

/*
 * Extract From host and convert it to lower case, the
 * result must be disposed using pkg_free after use
 */
static int get_to_host(str* res, struct sip_msg* msg)
{
	struct sip_uri puri;
	str uri;

	if (parse_headers(msg, HDR_TO_F, 0) < 0) {
		LOG(L_ERR, "domain: get_to_host: Error while parsing To header\n");
		return -1;
	}
	if (!msg->to) {
		LOG(L_ERR, "domain:get_to_host: No To header field found in message\n");
		return -1;
	}
	uri = get_to(msg)->uri;

	if (parse_uri(uri.s, uri.len, &puri) < 0) {
		LOG(L_ERR, "domain:get_to_host: Error while parsing From URI\n");
		return -3;
	}

	res->s = pkg_malloc(puri.host.len);
	if (!res->s) {
		LOG(L_ERR, "domain:get_to_host: No memory left\n");
		return -1;
	}
	memcpy(res->s, puri.host.s, puri.host.len);
	res->len = puri.host.len;
	strlower(res);
	return 0;
}


/*
 * Extract Request-URI host and convert it to lower case, the
 * result must be disposed using pkg_free after use
 */
static int get_ruri_host(str* res, struct sip_msg* msg)
{
	if (parse_sip_msg_uri(msg) < 0) {
	    LOG(L_ERR, "domain:get_ruri_host: Error while parsing URI\n");
	    return -1;
	}

	res->s = pkg_malloc(msg->parsed_uri.host.len);
	if (!res->s) {
		LOG(L_ERR, "domain:get_ruri_host: No memory left\n");
		return -1;
	}
	memcpy(res->s, msg->parsed_uri.host.s, msg->parsed_uri.host.len);
	res->len = msg->parsed_uri.host.len;
	strlower(res);
	return 0;
}


/*
 * Check if host in From uri is local
 */
static int is_from_local(struct sip_msg* msg, char* s1, char* s2)
{
	str host;
	int ret;

	if (get_from_host(&host, msg) < 0) return -1;
	ret = is_local(&host);
	pkg_free(host.s);
	return ret;
}


/*
 * Check if host in Request URI is local
 */
static int is_ruri_local(struct sip_msg* msg, char* s1, char* s2)
{
	int ret;
	str host;

	if (get_ruri_host(&host, msg) < 0) return -1;
	ret = is_local(&host);
	pkg_free(host.s);
	return ret;
}

static int get_host_from_avp(str* res, unsigned int* track, fparam_t* param)
{

	int_str val;
	
	if (search_first_avp(param->v.avp.flags, param->v.avp.name, &val, 0)) {
		*track = param->v.avp.flags & AVP_TRACK_ALL;
		if (!*track) *track = AVP_TRACK_FROM;

		res->s = pkg_malloc(val.s.len);
		if (!res->s) {
			ERR("No memory left\n");
			return -1;
		}
		memcpy(res->s, val.s.s, val.s.len);
		res->len = val.s.len;
		strlower(res);
		return 0;
	} else {
		return -1;
	}

}

static int lookup_domain(struct sip_msg* msg, char* s1, char* s2)
{
	fparam_t* param;
	str host;
	domain_t* d;
	unsigned int track;

	param = (fparam_t*)s1;
	track = 0;

	if (db_mode == 0) {
		LOG(L_ERR, "domain:lookup_domain only works in cache mode\n");
		return -1;
	}
	
	switch(param->type) {
	case FPARAM_AVP:
		if (get_host_from_avp(&host, &track, param) < 0) return -1;
		break;
		
	case FPARAM_STRING:
		switch(param->v.i) {
		case LOAD_FROM:
			if (get_from_host(&host, msg) < 0) return -1;
			track = AVP_TRACK_FROM;
			break;

		case LOAD_TO:
			if (get_to_host(&host, msg) < 0) return -1;
			track = AVP_TRACK_TO;
			break;

		case LOAD_RURI:
			if (get_ruri_host(&host, msg) < 0) return -1;
			track = AVP_TRACK_TO;
			break;
		}
		break;

	default:
		ERR("Unsupported parameter\n");
		return -1;
	}

	if (hash_lookup(&d, *active_hash, &host) == 1) {
		set_avp_list(AVP_CLASS_DOMAIN | track, &d->attrs);
		pkg_free(host.s);
		return 1;
	} else {
		pkg_free(host.s);
		return -1;
	}
}


int reload_domain_list(void)
{
	struct hash_entry** new_table;
	domain_t** new_list;

	/* Choose new hash table and free its old contents */
	if (*active_hash == hash_1) {
		free_table(hash_2);
		new_table = hash_2;
		new_list = domains_2;
	} else {
		free_table(hash_1);
		new_table = hash_1;
		new_list = domains_1;
	}

	if (load_domains(new_list) < 0) goto error;
	if (gen_domain_table(new_table, *new_list) < 0) goto error;
	*active_hash = new_table;
	return 0;
 error:
	free_table(new_table);
	free_domain_list(*new_list);
	return -1;
}


static int lookup_domain_fixup(void** param, int param_no)
{
	int ret;
	fparam_t* fp;

	if (param_no == 1) {
		ret = fix_param(FPARAM_AVP, param);
		if (ret <= 0) return ret;

		ret = fix_param(FPARAM_STRING, param);
		if (ret <= 0) {
			fp = (fparam_t*)(*param);
			if (!strcasecmp(fp->orig, "Request-URI")) {
				fp->v.i = LOAD_RURI;
			} else if (!strcasecmp(fp->orig, "From")) {
				fp->v.i = LOAD_FROM;
			} else if (!strcasecmp(fp->orig, "To")) {
				fp->v.i = LOAD_TO;
			}
		} else {
			ERR("Unknown parameter\n");
			return -1;
		}
	}

	return 0;
}
