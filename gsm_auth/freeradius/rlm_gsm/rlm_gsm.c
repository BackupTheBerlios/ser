/*
 * rlm_gsm.c
 *
 * Version:  $Id: rlm_gsm.c,v 1.1 2003/12/09 12:32:28 dcm Exp $
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Copyright 2002  The FreeRADIUS server project
 * Copyright 2002  FhG FOKUS <http://www.fokus.fraunhofer.de>
 */

#include "autoconf.h"
#include "libradius.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mysql.h>

#include "radiusd.h"
#include "modules.h"
#include "conffile.h"
#include "rad_assert.h"

/* Service types */
#define PW_GSM_AUTH             23

/*      Merit Experimental Extensions */
#ifndef PW_USER_REALM
#define PW_USER_REALM			223     /* string */
#endif

#define MAX_QUERY_LEN	512

static const char rcsid[] = "$Id: rlm_gsm.c,v 1.1 2003/12/09 12:32:28 dcm Exp $";

typedef struct _rlm_gsm_t
{
	int   service_type;
	MYSQL db_con;
	char  *mysql_host;
	int   mysql_port;
	char  *mysql_user;
	char  *mysql_passwd;
	char  *mysql_db;
	char  *mysql_table;
	char  *mysql_imsi;
	char  *mysql_realm;
	char  *mysql_challenge;
	char  *mysql_response;
	char  *mysql_key;
	char  *mysql_access;
} rlm_gsm_t;

static CONF_PARSER module_config[] = {
  { "service_type", PW_TYPE_INTEGER,
		  offsetof(rlm_gsm_t, service_type), NULL, "23"},
  { "mysql_host", PW_TYPE_STRING_PTR,
		  offsetof(rlm_gsm_t, mysql_host), NULL,   "localhost" },
  { "mysql_port", PW_TYPE_INTEGER,
		  offsetof(rlm_gsm_t, mysql_port), NULL, "3306"},
  { "mysql_user", PW_TYPE_STRING_PTR,
		  offsetof(rlm_gsm_t, mysql_user), NULL,   "root" },
  { "mysql_password", PW_TYPE_STRING_PTR,
		  offsetof(rlm_gsm_t, mysql_passwd), NULL,   "" },
  { "mysql_database", PW_TYPE_STRING_PTR,
		  offsetof(rlm_gsm_t, mysql_db), NULL,   "gsm_auth" },
  { "mysql_table", PW_TYPE_STRING_PTR,
		  offsetof(rlm_gsm_t, mysql_table), NULL,   "gsm_user" },
  { "mysql_imsi", PW_TYPE_STRING_PTR,
		  offsetof(rlm_gsm_t, mysql_imsi), NULL,   "imsi" },
  { "mysql_realm", PW_TYPE_STRING_PTR,
		  offsetof(rlm_gsm_t, mysql_realm), NULL,   "realm" },
  { "mysql_challenge", PW_TYPE_STRING_PTR,
		  offsetof(rlm_gsm_t, mysql_challenge), NULL,   "challenge" },
  { "mysql_response", PW_TYPE_STRING_PTR,
		  offsetof(rlm_gsm_t, mysql_response), NULL,   "response" },
  { "mysql_key", PW_TYPE_STRING_PTR,
		  offsetof(rlm_gsm_t, mysql_key), NULL,   "session_key" },
  { "mysql_access_time", PW_TYPE_STRING_PTR,
		  offsetof(rlm_gsm_t, mysql_access), NULL,   "access_time" },

  { NULL, -1, 0, NULL, NULL }		/* end the list */
};

/*
 *	Do any per-module initialization that is separate to each
 *	configured instance of the module.  e.g. set up connections
 *	to external databases, read configuration files, set up
 *	dictionary entries, etc.
 *
 *	If configuration information is given in the config section
 *	that must be referenced in later calls, store a handle to it
 *	in *instance otherwise put a null pointer there.
 */
static int gsm_instantiate(CONF_SECTION *conf, void **instance)
{
	rlm_gsm_t *data;
	
	/*
	 *	Set up a storage area for instance data
	 */
	data = rad_malloc(sizeof(*data));
	if (!data) {
		return -1;
	}
	memset(data, 0, sizeof(*data));

	/*
	 *	If the configuration parameters can't be parsed, then
	 *	fail.
	 */
	if (cf_section_parse(conf, data, module_config) < 0) {
		free(data);
		return -1;
	}

	/*
	 * connect to database
	 */
	mysql_init(&data->db_con);
	if(!mysql_real_connect(&data->db_con, data->mysql_host,
			data->mysql_user, data->mysql_passwd,
			data->mysql_db, data->mysql_port, NULL, 0))
	{
		DEBUG("gsm_instantiate:ERROR: cannot connect to mysql database [%s]",
				mysql_error(&data->db_con));
		return -1;
	}
	
	*instance = data;
	DEBUG("gsm_instantiate: done ...");
	
	return 0;
}

static int gsm_detach(void *instance)
{
	rlm_gsm_t* data = (rlm_gsm_t*)instance;
	/*
	 * close mysql connection
	 */
	mysql_close(&data->db_con);

	/*
	 * free allocated structures
	 */
	if(data->mysql_host)
		free(data->mysql_host);
	if(data->mysql_user)
		free(data->mysql_user);
	if(data->mysql_passwd)
		free(data->mysql_passwd);
	if(data->mysql_db)
		free(data->mysql_db);
	if(data->mysql_table)
		free(data->mysql_table);
	if(data->mysql_imsi)
		free(data->mysql_imsi);
	if(data->mysql_realm)
		free(data->mysql_realm);
	if(data->mysql_challenge)
		free(data->mysql_challenge);
	if(data->mysql_key)
		free(data->mysql_key);
	if(data->mysql_response)
		free(data->mysql_response);
	if(data->mysql_access)
		free(data->mysql_access);
	
	free(data);
	
	return 0;
}


static int gsm_authorize(void *instance, REQUEST *request)
{
	VALUE_PAIR *vp;
	VALUE_PAIR *reply;
	rlm_gsm_t* data = (rlm_gsm_t*)instance;
	char query[MAX_QUERY_LEN];
	MYSQL_RES *db_res;
	MYSQL_ROW db_row;
	unsigned long *db_len;

	/* quiet the compiler */
	instance = instance;

	/*
	 *	We need both of these attributes to do the authentication.
	 */
	vp = pairfind(request->packet->vps, PW_SERVICE_TYPE);
	if (vp == NULL || vp->lvalue!=data->service_type) {
		DEBUG("gsm_authorize: Not for GSM authentication module");
		return RLM_MODULE_INVALID;
	}

	/* check username */
	vp = pairfind(request->packet->vps, PW_USER_NAME);
	if (!vp) {
		DEBUG("gsm_authorize:ERROR: No GSM-User-Name: Cannot perform"
				" GSM authentication");
		return RLM_MODULE_INVALID;
	}

	/*
	 * build query
	 */
	strcpy(query,"SELECT ");
	strcat(query, data->mysql_challenge);
	strcat(query, ",");
	strcat(query, data->mysql_response);
	strcat(query, " FROM ");
	strcat(query, data->mysql_table);
	strcat(query, " WHERE ");
	strcat(query, data->mysql_imsi);
	strcat(query, "='");
	strncat(query, vp->strvalue, vp->length);
	strcat(query, "'");

	/* check realm */
	vp = pairfind(request->packet->vps, PW_USER_REALM);
	if (!vp) {
		DEBUG("gsm_authorize:ERROR: No GSM-Realm: Cannot perform"
				" GSM authentication");
		return RLM_MODULE_INVALID;
	}

	strcat(query, " AND ");
	strcat(query, data->mysql_realm);
	strcat(query, "='");
	strncat(query, vp->strvalue, vp->length);
	strcat(query, "'");

	printf("rlm_gsm:gsm_authorize: query [%s]\n", query);
	
	if(mysql_real_query(&data->db_con, query, strlen(query)))
	{
		DEBUG("gsm_authorize:ERROR: making MySQL query");
		return RLM_MODULE_FAIL;
	}
	
	db_res = mysql_store_result(&data->db_con);
	if(!db_res)
	{ /* IMSI doesn't exit in database */
		DEBUG("gsm_authorize:ERROR: no result for this user");
		return RLM_MODULE_NOTFOUND;
	}
	db_row = mysql_fetch_row(db_res);
	if(!db_row)
	{ /* IMSI doesn't exit in database */
		DEBUG("gsm_authorize:ERROR: no row for this user");
		mysql_free_result(db_res);
		return RLM_MODULE_NOTFOUND;
	}
	db_len = mysql_fetch_lengths(db_res);
	
	/* we have to generate a challenge ? */
	vp = pairfind(request->packet->vps, PW_USER_PASSWORD);
	if (!vp) {
		DEBUG("gsm_authorize: No GSM-Response - generate challenge");
	
		reply = paircreate(PW_CHAP_CHALLENGE, PW_TYPE_STRING);
		memcpy(&reply->strvalue[0], db_row[0], db_len[0]);
		reply->strvalue[db_len[0]] = '\0';
		reply->length = db_len[0];
		pairadd(&request->reply->vps, reply);
		/*
		 *  Mark the packet as an Access-Challenge packet.
		 *  The server will take care of sending it to the user.
		 */
		request->reply->code = PW_ACCESS_CHALLENGE;
		DEBUG("gsm_authorize: Sending back Access-Challenge.");
		mysql_free_result(db_res);
		return RLM_MODULE_HANDLED;
	}

	/*
	 * Check the GSM response
	 */
	if(vp->length != db_len[1]
		|| memcmp(vp->strvalue, db_row[1], vp->length)!=0)
	{
		DEBUG("gsm_authorize: GSM response does't match");
		mysql_free_result(db_res);
		return RLM_MODULE_REJECT;
	}

	/*
	 *	Check the GSM-Challenge 
	 */
	vp = pairfind(request->packet->vps, PW_CHAP_CHALLENGE);
	if (!vp) {
		DEBUG("gsm_authorize:ERROR: No GSM-Nonce - cannot perform"
				" GSM authentication");
		mysql_free_result(db_res);
		return RLM_MODULE_INVALID;
	}

	if(vp->length != db_len[0]
		|| memcmp(vp->strvalue, db_row[0], vp->length)!=0)
	{
		DEBUG("gsm_authenticate:ERROR: GSM challenge does't match");
		mysql_free_result(db_res);
		return RLM_MODULE_REJECT;
	}
	
	/*
	 *	Everything's OK, add a gsm authentication type.
	 */
	if (pairfind(request->config_items, PW_AUTHTYPE) == NULL) {
		DEBUG("rlm_gsm:gsm_authorize: Adding Auth-Type = GSM");
		pairadd(&request->config_items,
			pairmake("Auth-Type", "GSM", T_OP_EQ));
	}

	mysql_free_result(db_res);
	/*
	 * build query - update access time
	 */
	strcpy(query,"UPDATE ");
	strcat(query, data->mysql_table);
	strcat(query, " SET ");
	strcat(query, data->mysql_access);
	strcat(query, "=NOW()");
	if(mysql_real_query(&data->db_con, query, strlen(query)))
	{
		DEBUG("gsm_authorize:ERROR: updating access time");
		return RLM_MODULE_FAIL;
	}

	return RLM_MODULE_OK;
}

/*
 *	Perform all of the wondrous variants of gsm authentication.
 *	- it does nothing - everything was done in gsm_authorize
 */
static int gsm_authenticate(void *instance, REQUEST *request)
{
	return RLM_MODULE_OK;
}

/*
 *	The module name should be the only globally exported symbol.
 *	That is, everything else should be 'static'.
 *
 *	If the module needs to temporarily modify it's instantiation
 *	data, the type should be changed to RLM_TYPE_THREAD_UNSAFE.
 *	The server will then take care of ensuring that the module
 *	is single-threaded.
 */
module_t rlm_gsm = {
	"GSM",	
	0,				/* type */
	NULL,				/* initialization */
	gsm_instantiate,	/* instantiation */
	{
		gsm_authenticate,	/* authentication */
		gsm_authorize, 	/* authorization */
		NULL,			/* preaccounting */
		NULL,			/* accounting */
		NULL,			/* checksimul */
		NULL,			/* pre-proxy */
		NULL,			/* post-proxy */
		NULL			/* post-auth */
	},
	gsm_detach,			/* detach */
	NULL,				/* destroy */
};
