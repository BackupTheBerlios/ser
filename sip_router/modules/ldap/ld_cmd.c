/*
 * $Id: ld_cmd.c,v 1.10 2008/08/13 12:57:05 gkovacs Exp $
 *
 * LDAP Database Driver for SER
 *
 * Copyright (C) 2008 iptelorg GmbH
 *
 * This file is part of SER, a free SIP server.
 *
 * SER is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * SER is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

/** \addtogroup ldap
 * @{
 */

/** \file
 * Implementation of functions related to database commands.
 */

#include "ld_cmd.h"
#include "ld_fld.h"
#include "ld_con.h"
#include "ld_mod.h"
#include "ld_uri.h"
#include "ld_cfg.h"
#include "ld_res.h"

#include "../../mem/mem.h"
#include "../../dprint.h"
#include "../../ut.h"

#include <string.h>


/** Destroys a ld_cmd structure.
 * This function frees all memory used by ld_cmd structure.
 * @param cmd A pointer to generic db_cmd command being freed.
 * @param payload A pointer to ld_cmd structure to be freed.
 */
static void ld_cmd_free(db_cmd_t* cmd, struct ld_cmd* payload)
{
	db_drv_free(&payload->gen);
	if (payload->result) pkg_free(payload->result);
	pkg_free(payload);
}


static int build_result_array(char*** res, db_cmd_t* cmd)
{
	struct ld_fld* lfld;
	char** t;
	int i;
	if (cmd->result_count == 0) {
		*res = NULL;
		return 0;
	}

	t = (char**)pkg_malloc(sizeof(char*) * (cmd->result_count + 1));
	if (t == NULL) {
		ERR("ldap: No memory left\n");
		return -1;
	}
	t[cmd->result_count] = NULL;

	for(i = 0; i < cmd->result_count; i++) {
		lfld = DB_GET_PAYLOAD(cmd->result + i);
		/* Attribute names are always zero terminated */
		t[i] = lfld->attr.s;
	}

	*res = t;
	return 0;
}


int ld_cmd(db_cmd_t* cmd)
{
	struct ld_cmd* lcmd;
	struct ld_cfg* cfg;

	lcmd = (struct ld_cmd*)pkg_malloc(sizeof(struct ld_cmd));
	if (lcmd == NULL) {
		ERR("ldap: No memory left\n");
		goto error;
	}
	memset(lcmd, '\0', sizeof(struct ld_cmd));
	if (db_drv_init(&lcmd->gen, ld_cmd_free) < 0) goto error;

	switch(cmd->type) {
	case DB_PUT:
	case DB_DEL:
	case DB_UPD:
		ERR("ldap: The driver does not support directory modifications yet.\n");
		goto error;
		break;

	case DB_GET:
		break;

	case DB_SQL:
		ERR("ldap: The driver does not support raw queries yet.\n");
		goto error;
	}

	cfg = ld_find_cfg(&cmd->table);
	if (cfg == NULL) {
		ERR("ldap: Cannot find configuration for '%.*s', giving up\n",
			STR_FMT(&cmd->table));
		goto error;
	}

	lcmd->base = cfg->base.s;
	lcmd->scope = cfg->scope;

	lcmd->sizelimit = cfg->sizelimit;
	if (cfg->timelimit) {
		lcmd->timelimit.tv_sec = cfg->timelimit;
		lcmd->timelimit.tv_usec = 0;
	}

	if (cfg->filter.s) {
		lcmd->filter = cfg->filter;
	}

	if (ld_resolve_fld(cmd->match, cfg) < 0) goto error;
	if (ld_resolve_fld(cmd->result, cfg) < 0) goto error;

	if (build_result_array(&lcmd->result, cmd) < 0) goto error;
	DB_SET_PAYLOAD(cmd, lcmd);
	return 0;

 error:
	if (lcmd) {
		DB_SET_PAYLOAD(cmd, NULL);
		db_drv_free(&lcmd->gen);
		if (lcmd->result) pkg_free(lcmd->result);
		pkg_free(lcmd);
	}
	return -1;
}


int ld_cmd_exec(db_res_t* res, db_cmd_t* cmd)
{
	db_con_t* con;
	struct ld_res* lres;
	struct ld_cmd* lcmd;
	struct ld_con* lcon;
	char* filter, *err_desc;
	int ret, err;
	LDAPMessage *msg, *resmsg;
	int reconn_cnt = glb_reconn_cnt;
	int msgid;
	char *oid;
	struct berval *data;


	filter = NULL;
	err_desc = NULL;
	resmsg = NULL;

	/* First things first: retrieve connection info from the currently active
	 * connection and also mysql payload from the database command
	 */
	con = cmd->ctx->con[db_payload_idx];
	lcmd = DB_GET_PAYLOAD(cmd);
	lcon = DB_GET_PAYLOAD(con);

	if (ld_fld2ldap(&filter, cmd->match, &lcmd->filter) < 0) {
		ERR("ldap: Error while building LDAP search filter\n");
		goto error;
	}

	do {
		if (lcon->flags & LD_CONNECTED) {
			ret = ldap_search_ext(lcon->con, lcmd->base, lcmd->scope, filter,
								  lcmd->result, 0, NULL, NULL,
								  lcmd->timelimit.tv_sec ? &lcmd->timelimit : NULL,
								  lcmd->sizelimit,
								  &msgid);
			if (ret != LDAP_SUCCESS) {
				ERR("ldap: Error while searching: %s\n", ldap_err2string(ret));
				goto error;
			}
			ret = ldap_result(lcon->con,
							  LDAP_RES_ANY,
							  LDAP_MSG_ALL,
							  NULL,
							  &resmsg);
		} else {
			/* force it to reconnect */
			ret = -1;
		}

		if (ret <= 0) {
			ERR("ldap: Error in ldap_search: %s\n", ldap_err2string(ret));
			if (ret == LDAP_SERVER_DOWN) {
				lcon->flags &= ~LD_CONNECTED;
				do {
					if (!reconn_cnt) {
						ERR("ldap: maximum reconnection attempt reached! giving up\n");
						goto error;
					}
					reconn_cnt--;
					err = ld_con_connect(con);
				} while (err != 0);
			} else {
				goto error;
			}
		}
	} while (ret <= 0);


	/* looking for unsolicited messages */
	for (msg = ldap_first_message(lcon->con, resmsg);
		 msg != NULL;
		 msg = ldap_next_message(lcon->con, msg)) {
		if (ldap_msgtype(msg) == LDAP_RES_EXTENDED) {
			if (ldap_parse_extended_result(lcon->con,
										   msg,
										   &oid,
										   &data,
										   0) != LDAP_SUCCESS) {
				ERR("ldap: Error while parsing extended result\n");
 				goto error;
			}
			if (oid != NULL) {
				if (strcmp(oid, LDAP_NOTICE_OF_DISCONNECTION) == 0) {
					WARN("ldap: Notice of Disconnection (OID: %s)\n", oid);
				} else {
					WARN("ldap: Unsolicited message received. OID: %s\n", oid);
				}
				ldap_memfree(oid);
			}
			if (data != NULL) {
				WARN("ldap: Unsolicited message data: %.*s\n",
					 (int)data->bv_len, data->bv_val);
				ber_bvfree(data);
			}
		}
	}

	ret = ldap_parse_result(lcon->con, resmsg, &err, NULL, &err_desc, NULL, NULL, 0);
	if (ret != LDAP_SUCCESS) {
		ERR("ldap: Error while reading result status: %s\n",
			ldap_err2string(ret));
		goto error;
	}

	if (err != LDAP_SUCCESS) {
		ERR("ldap: LDAP server reports error: %s\n", ldap_err2string(err));
		goto error;
	}

	if (res) {
		lres = DB_GET_PAYLOAD(res);
		lres->msg = resmsg;
	} else if (resmsg) {
		ldap_msgfree(resmsg);
	}

	if (filter) pkg_free(filter);
	if (err_desc) ldap_memfree(err_desc);
	return 0;

 error:
	if (filter) pkg_free(filter);
	if (resmsg) ldap_msgfree(resmsg);
	if (err_desc) ldap_memfree(err_desc);
	return -1;
}


/* Iterate to the next search result in the linked list
 * of messages returned by the LDAP server and convert
 * the field values.
 */
static int search_entry(db_res_t* res, int init)
{
	db_con_t* con;
	struct ld_res* lres;
	struct ld_con* lcon;

	lres = DB_GET_PAYLOAD(res);
	/* FIXME */
	con = res->cmd->ctx->con[db_payload_idx];
	lcon = DB_GET_PAYLOAD(con);

	if (init
	    || !lres->current
	    || ldap_msgtype(lres->current) != LDAP_RES_SEARCH_ENTRY
	    /* there is no more value combination result left */
	    || ld_incindex(res->cmd->result)) {

		if (init)
			lres->current = ldap_first_message(lcon->con, lres->msg);
		else
			lres->current = ldap_next_message(lcon->con, lres->current);

		while(lres->current) {
			if (ldap_msgtype(lres->current) == LDAP_RES_SEARCH_ENTRY) {
				break;
			}
			lres->current = ldap_next_message(lcon->con, lres->current);
		}
		if (lres->current == NULL) return 1;
		if (ld_ldap2fldinit(res->cmd->result, lcon->con, lres->current) < 0) return -1;
	} else {
		if (ld_ldap2fld(res->cmd->result, lcon->con, lres->current) < 0) return -1;
	}

	res->cur_rec->fld = res->cmd->result;
	return 0;
}


int ld_cmd_first(db_res_t* res)
{
	return search_entry(res, 1);
}


int ld_cmd_next(db_res_t* res)
{
	return search_entry(res, 0);
}


/** @} */
