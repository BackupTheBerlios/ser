/*
 * $Id: sipCommonCfgTable.c,v 1.2 2002/09/19 12:23:54 jku Rel $
 *
 * SNMP Module
 *
 * Note: this file originally auto-generated by mib2c using
 * mib2c.sipdataset.conf
 *
 * Copyright (C) 2001-2003 Fhg Fokus
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
 */


#include "snmp_mod.h"
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>
/* for SIP_VERSION. XXX: Probably this should be handled from core?? */
#include "../../parser/msg_parser.h"	

/* Table sipCommonCfgTable */
static netsnmp_table_data_set* sipCommonCfgTable;

static int initialize_table_sipCommonCfgTable();
static Netsnmp_Node_Handler sipCommonCfgTable_handler;

/* The dynamic handler table */
static struct sip_snmp_handler** sipCommonCfgTable_h;
/* The global handler (if owner decides to handle this table that way) */
static struct sip_snmp_handler* sipCommonCfgTable_gh;
/* The registration function and friends */
static int sipCommonCfgTable_reg(struct sip_snmp_handler *h, int op);
static int sipCommonCfgTable_addObj(struct sip_snmp_handler *h, int col);
static int sipCommonCfgTable_newRow(struct sip_snmp_handler *h);

/* The handlers. The ones with prefix ser_ are just placeholders. The
 * real ones should be in ser core (or somewhere else that knows about the
 * object) */
static int ser_handleStatus(struct sip_snmp_obj *o, enum handler_op op);
static int ser_handleOrg(struct sip_snmp_obj *o, enum handler_op op);
/* May need to be moved along with handleStatus */
static int getLastChange(struct sip_snmp_obj *o, enum handler_op op);
static int getStartTime(struct sip_snmp_obj *o, enum handler_op op);

/* need them global because both getLastChange() and handleStatus() need them */
/* TODO: Maybe have a function to get/set them?? */
static struct timeval lastChange;
static u_long snmpLastChange=-1;

/** Initializes the sipCommonCfgTable module */
int init_sipCommonCfgTable()
{
	const char *func = "snmp_mod";
	/* here we initialize all the tables we're planning on supporting */
	if(initialize_table_sipCommonCfgTable() == -1) {
		LOG(L_ERR, "%s: Failed creating table sipCommonCfgTable\n", func);
		return -1;
	}
	return 0;
}

/** Initialize the sipCommonCfgTable table by defining it's contents and 
 * how it's structured */
static int initialize_table_sipCommonCfgTable()
{
	static oid sipCommonCfgTable_oid[] = {1,3,6,1,2,1,9990,1,1,1};
	size_t sipCommonCfgTable_oid_len = OID_LENGTH(sipCommonCfgTable_oid);
	const char *func = "snmp_mod";

	/* create the table structure itself */
	sipCommonCfgTable = netsnmp_create_table_data_set("sipCommonCfgTable");
	if(!sipCommonCfgTable) {
		LOG(L_ERR, "%s: Error creating table\n", func);
		return -1;
	}
	
	/* don't support creation of rows. Default, but just so i remember... */
	sipCommonCfgTable->allow_creation = 0;

	/***************************************************
	 * Adding indexes
	 */
	netsnmp_table_dataset_add_index(sipCommonCfgTable, ASN_INTEGER);

	/* column, type, writable?, default value, default value's length */
	netsnmp_table_set_multi_add_default_row(
		sipCommonCfgTable,
		COLUMN_SIPPROTOCOLVERSION, ASN_OCTET_STR, 0, NULL, 0,
		COLUMN_SIPSERVICEOPERSTATUS, ASN_INTEGER, 0, NULL, 0,
		/* XXX: Has dummy SET code. Chk ser_handleStatus() */
		COLUMN_SIPSERVICEADMINSTATUS, ASN_INTEGER, 1, NULL, 0,
		COLUMN_SIPSERVICESTARTTIME, ASN_TIMETICKS, 0, NULL, 0,
		COLUMN_SIPSERVICELASTCHANGE, ASN_TIMETICKS, 0, NULL, 0,
		/* XXX: Has dummy SET code. Chk serChangeOrg() */
		COLUMN_SIPORGANIZATION, ASN_OCTET_STR, 1, NULL, 0,
		COLUMN_SIPMAXSESSIONS, ASN_UNSIGNED, 0, NULL, 0,
		COLUMN_SIPREQUESTURIHOSTMATCHING, ASN_INTEGER, 1, NULL, 0,
		0);
	
	/* registering the table with the master agent */
	/* note: if you don't need a subhandler to deal with any aspects
	 * of the request, change sipCommonCfgTable_handler to "NULL" */
	netsnmp_register_table_data_set(
			netsnmp_create_handler_registration(
				"sipCommonCfgTable", 
				sipCommonCfgTable_handler,
				sipCommonCfgTable_oid,
				sipCommonCfgTable_oid_len,
				HANDLER_CAN_RWRITE),
			sipCommonCfgTable, NULL);

	return 0;
}

/* Initializes the handler table. Returns the registration function for this 
 * table */
reg_handler init_sipCommonCfgTable_h()
{
	const char *func = "snmp_mod";
	sipCommonCfgTable_h = calloc(SIPCOMMONCFGTABLE_COLUMNS+1,
		sizeof(struct sip_snmp_handler*));
	if(!sipCommonCfgTable_h) {
		LOG(L_ERR, "%s: Error initializing handler table: %s\n", 
			func, strerror(errno));
		return NULL;
	}

	return sipCommonCfgTable_reg;
}

/* Registration function. Called by snmp_register_handler() to register
 * objects belonging to this table. See snmp_handler.h for values of
 * op */
static int sipCommonCfgTable_reg(struct sip_snmp_handler *h, int op)
{
	const char *func = "snmp_mod";
	int col, i;
	static int first = 1;
	register struct sip_snmp_handler *c;

	if(!h) { 
		LOG(L_ERR, "%s: attemp to register invalid handler\n", func);
		return -1;
	}

	if(op == REG_OBJ) {
		if(!h->sip_obj || !h->sip_obj->value.voidp) {
			LOG(L_ERR, "%s: attemp to register invalid handler\n", func);
			return -1;
		}
		col = h->sip_obj->col; 
		if(col <1 || col > SIPCOMMONCFGTABLE_COLUMNS) {
			LOG(L_ERR, "%s: attempt to register invalid column %d\n", 
				func, col);
			return -1;
		}

		/* add handler to table. We make copy to make everybody's life easier 
		 * (note that value is not copied) */
		if(!(sipCommonCfgTable_h[col] = snmp_clone_handler(h))) {
			LOG(L_ERR, "%s: Error registering handler: %s\n", func,
				strerror(errno));
			return -1;
		}

		if(sipCommonCfgTable_addObj(h, col) == -1) {
			LOG(L_ERR, "%s: Error adding new object to table\n", func);
			snmp_free_handler(sipCommonCfgTable_h[col]);
			sipCommonCfgTable_h[col] = NULL;
			return -1;
		}
	} else if(op == NEW_ROW) {
		if(!h->sip_obj || !h->sip_obj->value.voidp) {
			LOG(L_ERR, "%s: attemp to register invalid handler\n", func);
			return -1;
		}
		/* if there's global handler, no need to add the handlers */
		if(!sipCommonCfgTable_gh && first) {
			/* first time around, jump over indexes, and copy the handlers.
			 * (Indexes don't have handlers associated with them). The default
			 * index (e.g. applIndex) shouldn't be passed in, that's why
			 * we jump over one less index. */
			c = h;
			if(SIPCOMMONCFGTABLE_INDEXES > 1) {
				for(i=1; i<SIPCOMMONCFGTABLE_INDEXES; i++)
					c = c->next;
				col = 2;	/* 2nd or 3rd index will be at column 1 */
			} else
				col = 1;
			while(c) {
				/* if too many we ignore the rest */
				if(col > SIPCOMMONCFGTABLE_COLUMNS) {
					LOG(L_ERR, "%s: Attempt to add too many objects to "
						"table row\n", func);
					return -1;
				}
				c->sip_obj->col = col;
				if(!(sipCommonCfgTable_h[col] = snmp_clone_handler(c))) {
					LOG(L_ERR, "%s: Error registering handler: %s\n", func,
						strerror(errno));
					return -1;
				}
				c = c->next;
				col++;
			}
			first = 0;
		}
		/* add the data */
		if(sipCommonCfgTable_newRow(h) == -1) {
			LOG(L_ERR, "%s: Error creating new table\n", func);
			return -1;
		}
	} else if(op == REG_TABLE) {
		if(!sipCommonCfgTable_gh)
			sipCommonCfgTable_gh = snmp_clone_handler(h);
		else
			memcpy(sipCommonCfgTable_gh, h, sizeof(struct sip_snmp_handler));
		if(!sipCommonCfgTable_gh) {
			LOG(L_ERR, "%s: Error registering table handler\n", func);
			return -1;
		}
		/* it's not necessary for the caller to create this, but we need
		 * it to pass info down to the handling function */
		if(!sipCommonCfgTable_gh->sip_obj) {
			sipCommonCfgTable_gh->sip_obj = calloc(1, 
					sizeof(struct sip_snmp_obj));
			if(!sipCommonCfgTable_gh->sip_obj) {
				LOG(L_ERR, "%s: Error registering table handler\n", func);
				free(sipCommonCfgTable_gh);
				sipCommonCfgTable_gh = NULL;
				return -1;
			}
		}
	} else {
		LOG(L_ERR, "%s: Invalid operation %d\n", func, op);
		return -1;
	}

	/* voila! */
	return 0;
}

/* Adds a new object to the table. If the row doesn't exist is created. 
 * Assumes all objects go to the same row, and all the objects are
 * new (replacing is done silently) */
static int sipCommonCfgTable_addObj(struct sip_snmp_handler *h, int col)
{
	static netsnmp_table_row *row = NULL;
	int applIndex;	/* Default index for most tables */
	int first = 0;
	const char *func = "snmp_mod";

	if(!row) {	/* First time. Create row and add indexes */
		/* Get index (applIndex). First since we don't need to undo it
		 * but it's still possible that it fails (e.g. if we get called 
		 * at the wrong time) */
		applIndex = ser_getApplIndex();
		if(applIndex == -1) {
			LOG(L_ERR, "%s: Failed getting table index\n", func);
			return -1;
		}
		/* XXX: If table has more indexes init them here and add
		 * a similar call to row_add_index() as below */

		/* Create the row */
		row = netsnmp_create_table_data_row();
		if(!row) {
			LOG(L_ERR, "%s: failed creating table row\n", func);
			return -1;
		}

		/* add the index(es) to the table */
		if(!netsnmp_table_row_add_index(row, ASN_INTEGER, &applIndex,
			sizeof(applIndex))) {
			LOG(L_ERR, "%s: Error adding index to row\n", func);
			netsnmp_table_data_delete_row(row);
			row = NULL;
			return -1;
		}

		first = 1;
	}

	/* sanity checks */
	if(col < 1 || col > SIPCOMMONCFGTABLE_COLUMNS) {
		LOG(L_ERR, "%s: Invalid column %d to add new object\n", func, col);
		return -1;
	}
	if(!h || !h->sip_obj || !h->sip_obj->value.voidp) {
		LOG(L_ERR, "%s: Invalid object to add\n", func);
		return -1;
	}

	/* add object to row */
	if(netsnmp_set_row_column(row, col, ser_types[h->sip_obj->type], 
			h->sip_obj->value.voidp, h->sip_obj->val_len)
			!= SNMPERR_SUCCESS) {
		LOG(L_ERR, "%s: Error adding new object to table\n", func);
		return -1;
	}

	/* is it writable? */
	if(h->on_set)
		netsnmp_mark_row_column_writable(row, col, 1);

	/* If first time, add the row. Subsequent times don't need to do
	 * anything since the table just has a pointer to our local row.
	 * However, if indexes were to change then the row needs to be 
	 * replaced */
	if(first) {
		if(netsnmp_table_data_add_row(sipCommonCfgTable->table, row) != 
				SNMPERR_SUCCESS) {
			LOG(L_ERR, "%s: Error adding new row to table\n",func); 
			return -1;
		}
	}

	/* Fin */
	return 0;
}

/* Handles creation of new rows for the table */
static int sipCommonCfgTable_newRow(struct sip_snmp_handler *h)
{
	netsnmp_table_row *row;
	/* XXX: make sure applIndex is an index for your table */
	static int 	applIndex = -1;
	const char *func = "snmp_mod";
	register struct sip_snmp_handler *c;
	struct sip_snmp_obj *o;
	int i, col;

	/* get applIndex first */
	if(applIndex == -1) {
		applIndex = ser_getApplIndex();
		if(applIndex == -1) {
			LOG(L_ERR,"%s: Couldn't get application index, cannot create "
				"new row\n", func);
			return -1;
		}
	}

	/* create the row */
	row = netsnmp_create_table_data_row();
	if(!row) {
		LOG(L_ERR, "%s: Couldn't create new row, out of memory?\n", func);
		return -1;
	}

	/* add indexes */
	netsnmp_table_row_add_index(row, 
		ASN_INTEGER, &applIndex, sizeof(applIndex));
	c = h;
	for(i=1; i<SIPCOMMONCFGTABLE_INDEXES; i++) {
		if(!c) {
			LOG(L_ERR, "%s: Not enought indexes passed, need %d\n", func,
				SIPCOMMONCFGTABLE_INDEXES);
			netsnmp_table_data_delete_row(row);
			return -1;
		}
		if(!c->sip_obj || !c->sip_obj->value.voidp) {
			LOG(L_ERR, "%s: Invalid index passed\n", func);
			netsnmp_table_data_delete_row(row);
			return -1;
		}

		if(!netsnmp_table_row_add_index(row, ser_types[c->sip_obj->type],
			c->sip_obj->value.voidp, c->sip_obj->val_len)) {
			LOG(L_ERR, "%s: Error adding index to row\n", func);
			netsnmp_table_data_delete_row(row);
			return -1;
		}
		c = c->next;
	}
	/* add the data. We start from the last index, all the way to the
	 * end of the linked list */
	c = h;
	for(i=2; i<SIPCOMMONCFGTABLE_INDEXES; i++) 
		c = c->next;
	col = 1;
	while(c) {
		if(col > SIPCOMMONCFGTABLE_COLUMNS) {
			LOG(L_ERR, "%s: Too many columns for new row\n", func);
			netsnmp_table_data_delete_row(row);
			return -1;
		}
		o = c->sip_obj;
		if(!o || !o->value.voidp) {
			LOG(L_ERR, "%s: Invalid object to add to new row\n", func);
			netsnmp_table_data_delete_row(row);
			return -1;
		}
		if(netsnmp_set_row_column(row, col, ser_types[o->type],
			o->value.voidp, o->val_len) != SNMPERR_SUCCESS) {
			LOG(L_ERR, "%s: Error adding object to row\n", func);
			netsnmp_table_data_delete_row(row);
			return -1;
		}
		if(c->on_set)
			netsnmp_mark_row_column_writable(row, col, 1);

		/* next, please... */
		c = c->next;
		col++;
	}

	/* add the row to the table */
	if(netsnmp_table_data_add_row(sipCommonCfgTable->table, row) != 
			SNMPERR_SUCCESS) {
		LOG(L_ERR, "%s: Error adding new row to table\n", func);
		netsnmp_table_data_delete_row(row);
		return -1;
	}

	return 0;
}

/* only accepts read-only data */
int sipCommonCfgTable_replaceRow(
		struct sip_snmp_obj *idx,
		struct sip_snmp_obj *data)
{
	netsnmp_table_row *row;
	netsnmp_variable_list *idxs=NULL;
	struct sip_snmp_obj *o;
	int col;
	static int applIndex = -1;
	const char *func = "snmp_mod";

	if(applIndex == -1) {
		applIndex = ser_getApplIndex();
		if(applIndex == -1 ) {
			LOG(L_ERR, "%s: Error while looking for previous row\n", func);
			return -1;
		}
	}

	/* create index list */
	if(snmp_varlist_add_variable(&idxs, NULL, 0, ASN_INTEGER, 
			(u_char*)&applIndex, sizeof(applIndex)) == NULL) {
		LOG(L_ERR, "%s: Error while looking for previous row\n", func);
		return -1;
	}
	o = idx;
	while(o) {
		if(snmp_varlist_add_variable(&idxs, NULL, 0, ser_types[o->type],
			o->value.voidp, o->val_len) == NULL) {
			LOG(L_ERR, "%s: Error while looking for row to replace\n", func);
			snmp_free_var(idxs);
			return -1;
		}
		o = o->next;
	}

	/* find row */
	row = netsnmp_table_data_get(sipCommonCfgTable->table, idxs);
	if(!row) {
		LOG(L_ERR, "%s: Couldn't find row to replace\n", func);
		snmp_free_var(idxs);
		return -1;
	}
	snmp_free_var(idxs);

	/* add the data */
	o = data;
	col = 2;
	while(o) {
		if(col > SIPCOMMONCFGTABLE_COLUMNS) {
			LOG(L_ERR, "%s: Too many columns to add\n", func);
			return -1;
		}
		if(!o->value.voidp) {
			LOG(L_ERR, "%s: Invalid object to add\n", func);
			return -1;
		}
		o->col = col;
		/* all previous objects are left in the row */
		if(netsnmp_set_row_column(row, col, ser_types[o->type],
			o->value.voidp, o->val_len) != SNMPERR_SUCCESS) {
			LOG(L_ERR, "%s: Error adding object to row\n", func);
			return -1;
		}
		o=o->next;
		col++;
	}

	return 0;
}

/** handles requests for the sipCommonCfgTable table.
 * For every request it checks the specified object to see if it has a
 * handler, and calls it */
static int sipCommonCfgTable_handler(
		netsnmp_mib_handler               *handler,
		netsnmp_handler_registration      *reginfo,
		netsnmp_agent_request_info        *reqinfo,
		netsnmp_request_info              *requests) 
{
	netsnmp_variable_list *var;
	netsnmp_table_request_info *table_info;
	struct sip_snmp_handler *h;
	struct sip_snmp_obj *o;
	const char *func = "snmp_mod";
	int res;
	void *tmp_val;
	size_t tmp_len;

	while(requests) {
		var = requests->requestvb;
		if(requests->processed != 0)
			goto next;
		table_info = netsnmp_extract_table_info(requests);
		if(!table_info)
			goto next;
		/* this is not an error, since table-walks work by trying to get
		 * things until we run off of it */
		if(table_info->colnum > SIPCOMMONCFGTABLE_COLUMNS)
			goto next;

		/* Get the handler and its object */
		if(sipCommonCfgTable_gh) {
			h = sipCommonCfgTable_gh;
			/* sip_obj is valid since we create upon registration */
			h->sip_obj->opaque = (void*)sipCommonCfgTable_replaceRow;
		} else {
			h = sipCommonCfgTable_h[table_info->colnum];
			if(!h) 
				goto next;
		}
		o = h->sip_obj;
		if(!o) {	/* bad bad boy... */
			LOG(L_ERR, "%s: Found handler without an object!!!\n", func);
			goto next;
		}
		o->col = table_info->colnum;
		o->row = var->name[var->name_length-1];
		switch(reqinfo->mode) {
			case MODE_GET:
			case MODE_GETNEXT:
				if(!h->on_get) break;
				res = h->on_get(o, SER_GET);
				if(res == -1) {
					/* since we don't have a way of knowing what went wrong,
					 * just use a generic error code */
					netsnmp_set_request_error(reqinfo, requests,
							SNMP_ERR_RESOURCEUNAVAILABLE);
					break;
				} else if(res == 0)
					/* the handler has new value to pass back up */
					snmp_set_var_typed_value(var, ser_types[o->type],
							(u_char*)o->value.voidp, o->val_len);
				break;
			case MODE_SET_RESERVE1:
				/* NOTE: We don't require the handler for a on_reserve
				 * function since for our cases it seems that just
				 * checking the type is enough */

				/* First make sure handler wants SETs */
				if(!h->on_set) break;
				/* Check the type */
				if(requests->requestvb->type != ser_types[o->type]) {
					LOG(L_ERR, "%s: Wrong type on SET processing\n", func);
					netsnmp_set_request_error(reqinfo, requests,
							SNMP_ERR_WRONGTYPE);
					break;
				}
				break;
			case MODE_SET_ACTION: /* the real deal */
				if(!h->on_set) break;
				/* copy in the new value for the handler */
				tmp_val = o->value.voidp;
				tmp_len = o->val_len;
				o->value.voidp = requests->requestvb->val.string;
				o->val_len = requests->requestvb->val_len;
				if(h->on_set(o, SER_SET) == -1) {
					LOG(L_ERR, "%s: SET Handler for object failed\n", func);
					netsnmp_set_request_error(reqinfo, requests,
						SNMP_ERR_RESOURCEUNAVAILABLE);
					o->value.voidp = tmp_val;
					o->val_len = tmp_len;
					break;
				}
				o->value.voidp = tmp_val;
				o->val_len = tmp_len;
				break;
			case MODE_SET_UNDO:
				if(!h->on_end) {
					if(h->on_set) /*tsk, tsk, bad boy, gonna tell your mamma..*/
						LOG(L_ERR, "%s: Found object without UNDO handler\n",
							func);
					break;
				}
				/* no point in checking for errors since we're already on
				 * an error branch */
				h->on_end(o, SER_UNDO);
				break;
			case MODE_SET_COMMIT:
				/* Tell the handler is all good and it can safely free up
				 * any memory it may have allocated for UNDO */
				if(!h->on_end) 
					break;
				h->on_end(o, SER_COMMIT);
				break;
			case MODE_SET_FREE:
				/* We get here on failure from RESERVE1. Since there we only 
				 * chk for type correctness, there's nothing to do here */
				break;
		}
next:
		requests = requests->next;
	}
	return SNMP_ERR_NOERROR;
}

#define gimme_new(i, s) \
	i = snmp_new_handler(s); \
	if(!i) \
		goto error;	

/* registers a row in sipCommonCfgTable */
int ser_add_sipCommonCfgTable()
{
	const char *func = "snmp_mod";
	struct sip_snmp_handler *h, *f;
	struct sip_snmp_obj *o;

	gimme_new(f, 0)
	h = f;
	/* sipProtocolVersion */
	h->on_get = h->on_set = h->on_end = NULL;
	o = h->sip_obj;
	o->type = SER_STRING;
	o->value.string = strdup(SIP_VERSION);
	o->val_len = SIP_VERSION_LEN;

	/* sipServiceOperStatus */
	gimme_new(h->next, 0)
	h = h->next;
	h->on_get = ser_handleStatus;
	h->on_set = h->on_end = NULL;
	o = h->sip_obj;
	/* get default status */
	if(ser_handleStatus(o, SER_GET) == -1) 
		goto error;

	/* sipServiceAdminStatus */
	gimme_new(h->next, o->val_len)	/* same size as operStatus */
	h=h->next;
	h->on_get = h->on_set = h->on_end = ser_handleStatus;
	o = h->sip_obj;
	o->type = SER_INTEGER;
	*o->value.integer = 1;	/* default value is 1 (noop) */

	/* sipServiceStartTime */
	gimme_new(h->next, 0)
	h=h->next;
	h->on_get = getStartTime;
	h->on_set = h->on_end = NULL;
	o = h->sip_obj;
	if(getStartTime(o, SER_GET) == -1)
		goto error;

	/* sipServiceLastChange */
	gimme_new(h->next, 0)
	h = h->next;
	h->on_get = getLastChange;
	h->on_set = h->on_end = NULL;
	o = h->sip_obj;
	if(getLastChange(o, SER_GET) == -1)
		goto error;

	/* sipOrganization */
	gimme_new(h->next, 0)
	h = h->next;
	h->on_get = h->on_set = h->on_end = ser_handleOrg;
	o = h->sip_obj;
	if(ser_handleOrg(o, SER_GET) == -1)
		goto error;

	/* sipMaxSessions */
	gimme_new(h->next, sizeof(u_long))
	h = h->next;
	h->on_get = h->on_set = h->on_end = NULL;
	/* jiri's words:
	 * Hard to say, probably 0 is closest to the truth. We do not maintain 
	 * sessions. We either maintain 0-state with core or just transactions 
	 * with TM
	 */
	o = h->sip_obj;
	*o->value.integer = 0;
	o->type = SER_UNSIGNED;

	/* sipRequestUriHostMatching */
	/* XXX: Should be RW */
	gimme_new(h->next, sizeof(u_int))
	h = h->next;
	h->on_get = h->on_set = h->on_end = NULL;
	o = h->sip_obj;
	*o->value.integer = 0;
	o->type = SER_INTEGER;

	/* ok, now register */
	if(snmp_register_row("sipCommonCfgTable", f) == -1)
		goto error;

	/* hurray!! now free the memory */
	while(f) {
		h = f->next;
		snmp_free_handler(f);
		f = h;
	}
	
	return 0;

error:
	LOG(L_ERR, "%s: Error adding row to sipCommonCfgTable: %s\n",
			func, strerror(errno));
	while(f) {
		h = f->next;
		snmp_free_handler(f);
		f = h;
	}
	return -1;
}

/************************** HANDLER FUNCTIONS ***************************/
/* First two support only GET. The other two support all the operations */
/* 
 * The first time its called it computes the uptime, so try to call this 
 * function early on.
 *
 * from the MIB definition for sipServiceStartTime:
 * "The value of sysUpTime at the time the SIP entity was last started. If 
 * started prior to the last re-initialization of the local network management 
 * subsystem, then this object contains a zero value" 
 */
static int getStartTime(struct sip_snmp_obj *o, enum handler_op op)
{
	static struct timeval serUptime;
	static u_long snmpUptime=-1;
	u_long agentT, uptime;
	struct timeval tv, diff;
	const char *func = "snmp_mod";

	if(snmpUptime == -1) {	/* first time around */
		gettimeofday(&serUptime, NULL);
		/* this is the agent's uptime when ser is initialized, which is what
		 * we need to report */
		snmpUptime = netsnmp_get_agent_uptime();
	}
	
	if(op != SER_GET) {
		LOG(L_ERR, "%s: Invalid operation %d. Can only do GET\n", func, op);
		return -1;
	}

	/* formalities first */
	if(!o->value.integer)
		o->value.integer = calloc(1, sizeof(u_long));
	if(!o->value.integer) {
		LOG(L_ERR, "%s: %s\n", func, strerror(errno));
		return -1;
	}
	
	/* compute the thing */
	gettimeofday(&tv, NULL);
	/* formula from snmp agent (snmplib/tools). Computes in 1/100th seconds */
	diff.tv_sec = tv.tv_sec - serUptime.tv_sec - 1;
	diff.tv_usec = tv.tv_usec - serUptime.tv_usec + 1000000;
	
	/* this is our uptime */
	uptime = ((u_long) diff.tv_sec) * 100 + diff.tv_usec / 10000;
	agentT = netsnmp_get_agent_uptime();
	*o->value.integer = (uptime < agentT ? snmpUptime : 0);

	/* fill the rest and voila! */
	o->val_len = sizeof(snmpUptime);
	o->type = SER_TIMETICKS;

	return 0;
}

/* same as above */
static int getLastChange(struct sip_snmp_obj *o, enum handler_op op)
{
	u_long agentT, uptime;
	struct timeval tv, diff;
	const char *func = "snmp_mod";

	if(snmpLastChange == -1) {
		gettimeofday(&lastChange, NULL);
		snmpLastChange = netsnmp_get_agent_uptime();
	}

	if(op != SER_GET) {
		LOG(L_ERR, "%s: Invalid operation %d. Can only do GET\n", func, op);
		return -1;
	}

	/* formalities first */
	if(!o->value.integer)
		o->value.integer = calloc(1, sizeof(u_long));
	if(!o->value.integer) {
		LOG(L_ERR, "%s: %s\n", func, strerror(errno));
		return -1;
	}

	/* compute the thing */
	gettimeofday(&tv, NULL);
	/* formula is from snmp agent (snmplib/tools.c) */
	diff.tv_sec = tv.tv_sec - lastChange.tv_sec - 1;
    diff.tv_usec = tv.tv_usec - lastChange.tv_usec + 1000000;

	/* our uptime in 1/100th secs */
    uptime = ((u_long) diff.tv_sec) * 100 + diff.tv_usec / 10000;
	agentT = netsnmp_get_agent_uptime();

	*o->value.integer = (uptime < agentT ? snmpLastChange : 0);

	o->val_len = sizeof(snmpLastChange);
	o->type = SER_TIMETICKS;

	return 0;
}

/* Handles ADMINSTATUS and OPERSTATUS. It's just a dummy function.
 * The real one should be in ser core. This should be tied to
 * sipServiceLastChange */
/* GETs are for OPERSTATUS, SETs are for ADMINSTATUS */
/* Test with:
 * snmpset localhost sipServiceStatus.1 = 3 (3 -> down)
 * If running standalone, add -v 1 -c <your community> before localhost 
 */
static int ser_handleStatus(struct sip_snmp_obj *o, enum handler_op op)
{
	static u_int serStatus = -1;
	u_int newS;
	static u_int oldS =-1;	/* for undo */
	const char *func = "snmp_mod";

	/* first time around, set the status to up */
	if(serStatus == -1)
		serStatus = 1;

	switch(op) {
		case SER_GET:
			if(!o->value.integer)
				o->value.integer = calloc(1, sizeof(u_int));
			if(!o->value.integer) {
				LOG(L_ERR, "%s: %s\n", func, strerror(errno));
				return -1;
			}
			if(o->col == COLUMN_SIPSERVICEOPERSTATUS)
				*o->value.integer = serStatus;
			else
				/* for admin status always return noop(1), but this is
				 * just to play nice. We shouldn't get called for GET
				 * on ADMINSTATUS */
				*o->value.integer = 1;
			o->val_len = sizeof(serStatus);
			o->type = SER_INTEGER;
			break;
		case SER_SET:
			newS = *o->value.integer;
			if(newS == 1) {	/* do nothing */
				LOG(L_DBG, "%s: noop(1) state passed. Doing nothing\n", func);
				/* so we know nothing happened in case undo is called */
				oldS = -1;
				break;
			}
			/* chk validity */
			if(newS < 2 || newS > 6) {
				LOG(L_ERR, "%s: Can't change to invalid operation state %d\n",
					func, newS);
				return -1;
			}
			oldS = serStatus;
			serStatus = newS-1;
			
			/* update info for sipServiceLastChange */
			gettimeofday(&lastChange, NULL);
			snmpLastChange = netsnmp_get_agent_uptime();
			LOG(L_DBG, "%s: Changed ser's status to %d\n", func, serStatus);
			break;
		case SER_UNDO:
			/* Should we mess with lastChange too?? */
			if(oldS == -1)
				break;
			LOG(L_DBG, "%s: Undoing status change. Going back to %d\n", func,
				oldS);
			serStatus = oldS;
			break;
		case SER_COMMIT:	
			/* all good, we can forget about the backup. In this case
			 * doesn't make any difference since it's only an integer.
			 * Look at sipOrganization for something more useful */
			oldS = -1;
			break;
		default:
			LOG(L_ERR, "%s: Invalid operation for this handler\n", func);
			return -1;
	}

	return 0;
}

/* XXX: should be handled from core */
static int ser_handleOrg(struct sip_snmp_obj *o, enum handler_op op)
{
/* XXX: Is this defined somewhere in core?? */
#define DEF_SIPORG "iptel.org"
#define DEF_SIPORG_LEN 10
#define MAX_ORG_LEN 255	/* by definition.. */
	static char *sipOrganization = NULL, *old_org=NULL;
	char *new_org;
	static int org_len, old_len=0;
	const char *func = "snmp_mod";

	if(!sipOrganization) {
		sipOrganization = calloc(MAX_ORG_LEN+1, sizeof(char));
		old_org = calloc(MAX_ORG_LEN+1, sizeof(char));
		if(!sipOrganization || !old_org) {
			free(sipOrganization), free(old_org);
			LOG(L_ERR, "%s: %s\n", func, strerror(errno));
			return -1;
		}
		strncpy(sipOrganization, DEF_SIPORG, DEF_SIPORG_LEN);
		org_len = DEF_SIPORG_LEN;
		old_len = 0;
	}

	switch(op) {
		case SER_GET:
			if(!o->value.string) 
				o->value.string = calloc(org_len+1, sizeof(char));
			else 
				o->value.string = realloc(o->value.string, org_len+1);
			if(!o->value.string) {
					LOG(L_ERR, "%s: %s\n", func, strerror(errno));
					return -1;
			}
			strncpy(o->value.string, sipOrganization, org_len);
			o->val_len = org_len;
			o->type = SER_STRING;
			break;
		case SER_SET:
			if(o->val_len > MAX_ORG_LEN) {
				LOG(L_ERR, "%s: New sipOrganization too long (%d), max is %d\n",
					func, o->val_len, MAX_ORG_LEN);
				return -1;
			}
			strncpy(old_org,sipOrganization, org_len);
			old_len = org_len;
			new_org = o->value.string;
			org_len = o->val_len;
			strncpy(sipOrganization, new_org, org_len);
			sipOrganization[org_len] = '\0';
			LOG(L_DBG, "%s: Changed sipOrganization to %s, length %d\n", func,
				sipOrganization, org_len);
			break;
		case SER_UNDO:
			if(!old_len)
				break;
			LOG(L_DBG, "%s: undoing to old org %s\n", func, old_org);
			strncpy(sipOrganization, old_org, old_len);
			org_len = old_len;
			old_len = 0;
			break;
		case SER_COMMIT:
			old_len = 0;
			break;
		default:
			LOG(L_ERR, "%s: invalid operation for this handler\n", func);
			return -1;
	}
	return 0;
}
