/*
 * $Id: sipCommonCfgExpires.c,v 1.2 2002/09/19 12:23:54 jku Rel $
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
#include <stdlib.h>

/* Table sipCommonCfgExpiresStatusCodeTable */
static netsnmp_table_data_set* sipCommonCfgExpiresStatusCodeTable;

static int initialize_table_sipCommonCfgExpiresStatusCodeTable();
static Netsnmp_Node_Handler sipCommonCfgExpiresStatusCodeTable_handler;

/* The dynamic handler table */
static struct sip_snmp_handler** sipCommonCfgExpiresStatusCodeTable_h;
/* The global handler (if owner decides to handle this table that way) */
static struct sip_snmp_handler* sipCommonCfgExpiresStatusCodeTable_gh;
/* The registration function and friends */
static int sipCommonCfgExpiresStatusCodeTable_reg(struct sip_snmp_handler *h, int op);
static int sipCommonCfgExpiresStatusCodeTable_addObj(struct sip_snmp_handler *h, int col);
static int sipCommonCfgExpiresStatusCodeTable_newRow(struct sip_snmp_handler *h);

/* Table sipCommonCfgExpiresMethodTable */
static netsnmp_table_data_set* sipCommonCfgExpiresMethodTable;

static int initialize_table_sipCommonCfgExpiresMethodTable();
static Netsnmp_Node_Handler sipCommonCfgExpiresMethodTable_handler;

/* The dynamic handler table */
static struct sip_snmp_handler** sipCommonCfgExpiresMethodTable_h;
/* The global handler (if owner decides to handle this table that way) */
static struct sip_snmp_handler* sipCommonCfgExpiresMethodTable_gh;
/* The registration function and friends */
static int sipCommonCfgExpiresMethodTable_reg(struct sip_snmp_handler *h, int op);
static int sipCommonCfgExpiresMethodTable_addObj(struct sip_snmp_handler *h, int col);
static int sipCommonCfgExpiresMethodTable_newRow(struct sip_snmp_handler *h);

/** Initializes the sipCommonCfgExpires module */
int init_sipCommonCfgExpires()
{
	const char *func = "snmp_mod";
	/* here we initialize all the tables we're planning on supporting */
	if(initialize_table_sipCommonCfgExpiresStatusCodeTable() == -1) {
		LOG(L_ERR, "%s: Failed creating table sipCommonCfgExpiresStatusCodeTable\n", func);
		return -1;
	}
	if(initialize_table_sipCommonCfgExpiresMethodTable() == -1) {
		LOG(L_ERR, "%s: Failed creating table sipCommonCfgExpiresMethodTable\n", func);
		return -1;
	}
	return 0;
}

/** Initialize the sipCommonCfgExpiresStatusCodeTable table by defining it's contents and 
 * how it's structured */
static int initialize_table_sipCommonCfgExpiresStatusCodeTable()
{
	static oid sipCommonCfgExpiresStatusCodeTable_oid[] = {1,3,6,1,2,1,9990,1,4,2};
	size_t sipCommonCfgExpiresStatusCodeTable_oid_len = OID_LENGTH(sipCommonCfgExpiresStatusCodeTable_oid);
	const char *func = "snmp_mod";

	/* create the table structure itself */
	sipCommonCfgExpiresStatusCodeTable = netsnmp_create_table_data_set("sipCommonCfgExpiresStatusCodeTable");
	if(!sipCommonCfgExpiresStatusCodeTable) {
		LOG(L_ERR, "%s: Error creating table\n", func);
		return -1;
	}
	
	/***************************************************
	 * Adding indexes
	 */
	netsnmp_table_dataset_add_index(sipCommonCfgExpiresStatusCodeTable, ASN_INTEGER);
	netsnmp_table_dataset_add_index(sipCommonCfgExpiresStatusCodeTable, ASN_INTEGER);
	
	netsnmp_table_set_multi_add_default_row(
		sipCommonCfgExpiresStatusCodeTable,
		COLUMN_SIPCFGEXPIRESSTATUSCODEVALUE, ASN_INTEGER, 0, NULL, 0,
		COLUMN_SIPCFGEXPIRESHEADERVALUE, ASN_UNSIGNED, 1, NULL, 0,
		COLUMN_SIPCFGEXPIRESSTATUSCODESTATUS, ASN_INTEGER, 1, NULL, 0,
		0);
	
	/* registering the table with the master agent */
	/* note: if you don't need a subhandler to deal with any aspects
	 * of the request, change sipCommonCfgExpiresStatusCodeTable_handler to "NULL" */
	netsnmp_register_table_data_set(
			netsnmp_create_handler_registration(
				"sipCommonCfgExpiresStatusCodeTable", 
				sipCommonCfgExpiresStatusCodeTable_handler,
				sipCommonCfgExpiresStatusCodeTable_oid,
				sipCommonCfgExpiresStatusCodeTable_oid_len,
				HANDLER_CAN_RWRITE),
			sipCommonCfgExpiresStatusCodeTable, NULL);

	return 0;
}

/* Initializes the handler table. Returns the registration function for this 
 * table */
reg_handler init_sipCommonCfgExpiresStatusCodeTable_h()
{
	const char *func = "snmp_mod";
	sipCommonCfgExpiresStatusCodeTable_h = calloc(SIPCOMMONCFGEXPIRESSTATUSCODETABLE_COLUMNS+1,
		sizeof(struct sip_snmp_handler*));
	if(!sipCommonCfgExpiresStatusCodeTable_h) {
		LOG(L_ERR, "%s: Error initializing handler table: %s\n", 
			func, strerror(errno));
		return NULL;
	}

	return sipCommonCfgExpiresStatusCodeTable_reg;
}

/* Registration function. Called by snmp_register_handler() to register
 * objects belonging to this table. See snmp_handler.h for values of
 * op */
static int sipCommonCfgExpiresStatusCodeTable_reg(struct sip_snmp_handler *h, int op)
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
		if(col <1 || col > SIPCOMMONCFGEXPIRESSTATUSCODETABLE_COLUMNS) {
			LOG(L_ERR, "%s: attempt to register invalid column %d\n", 
				func, col);
			return -1;
		}

		/* add handler to table. We make copy to make everybody's life easier 
		 * (note that value is not copied) */
		if(!(sipCommonCfgExpiresStatusCodeTable_h[col] = snmp_clone_handler(h))) {
			LOG(L_ERR, "%s: Error registering handler: %s\n", func,
				strerror(errno));
			return -1;
		}

		if(sipCommonCfgExpiresStatusCodeTable_addObj(h, col) == -1) {
			LOG(L_ERR, "%s: Error adding new object to table\n", func);
			snmp_free_handler(sipCommonCfgExpiresStatusCodeTable_h[col]);
			sipCommonCfgExpiresStatusCodeTable_h[col] = NULL;
			return -1;
		}
	} else if(op == NEW_ROW) {
		if(!h->sip_obj || !h->sip_obj->value.voidp) {
			LOG(L_ERR, "%s: attemp to register invalid handler\n", func);
			return -1;
		}
		/* if there's global handler, no need to add the handlers */
		if(!sipCommonCfgExpiresStatusCodeTable_gh && first) {
			/* first time around, jump over indexes, and copy the handlers.
			 * (Indexes don't have handlers associated with them). The default
			 * index (e.g. applIndex) shouldn't be passed in, that's why
			 * we jump over one less index. */
			c = h;
			if(SIPCOMMONCFGEXPIRESSTATUSCODETABLE_INDEXES > 1) {
				for(i=1; i<SIPCOMMONCFGEXPIRESSTATUSCODETABLE_INDEXES; i++)
					c = c->next;
				col = 2;	/* 2nd or 3rd index will be at column 1 */
			} else
				col = 1;
			while(c) {
				/* if too many we ignore the rest */
				if(col > SIPCOMMONCFGEXPIRESSTATUSCODETABLE_COLUMNS) {
					LOG(L_ERR, "%s: Attempt to add too many objects to "
						"table row\n", func);
					return -1;
				}
				c->sip_obj->col = col;
				if(!(sipCommonCfgExpiresStatusCodeTable_h[col] = snmp_clone_handler(c))) {
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
		if(sipCommonCfgExpiresStatusCodeTable_newRow(h) == -1) {
			LOG(L_ERR, "%s: Error creating new table\n", func);
			return -1;
		}
	} else if(op == REG_TABLE) {
		if(!sipCommonCfgExpiresStatusCodeTable_gh)
			sipCommonCfgExpiresStatusCodeTable_gh = snmp_clone_handler(h);
		else
			memcpy(sipCommonCfgExpiresStatusCodeTable_gh, h, sizeof(struct sip_snmp_handler));
		if(!sipCommonCfgExpiresStatusCodeTable_gh) {
			LOG(L_ERR, "%s: Error registering table handler\n", func);
			return -1;
		}
		/* it's not necessary for the caller to create this, but we need
		 * it to pass info down to the handling function */
		if(!sipCommonCfgExpiresStatusCodeTable_gh->sip_obj) {
			sipCommonCfgExpiresStatusCodeTable_gh->sip_obj = calloc(1, 
					sizeof(struct sip_snmp_obj));
			if(!sipCommonCfgExpiresStatusCodeTable_gh->sip_obj) {
				LOG(L_ERR, "%s: Error registering table handler\n", func);
				free(sipCommonCfgExpiresStatusCodeTable_gh);
				sipCommonCfgExpiresStatusCodeTable_gh = NULL;
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
static int sipCommonCfgExpiresStatusCodeTable_addObj(struct sip_snmp_handler *h, int col)
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
	if(col < 1 || col > SIPCOMMONCFGEXPIRESSTATUSCODETABLE_COLUMNS) {
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
		if(netsnmp_table_data_add_row(sipCommonCfgExpiresStatusCodeTable->table, row) != 
				SNMPERR_SUCCESS) {
			LOG(L_ERR, "%s: Error adding new row to table\n",func); 
			return -1;
		}
	}

	/* Fin */
	return 0;
}

/* Handles creation of new rows for the table */
static int sipCommonCfgExpiresStatusCodeTable_newRow(struct sip_snmp_handler *h)
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
	for(i=1; i<SIPCOMMONCFGEXPIRESSTATUSCODETABLE_INDEXES; i++) {
		if(!c) {
			LOG(L_ERR, "%s: Not enought indexes passed, need %d\n", func,
				SIPCOMMONCFGEXPIRESSTATUSCODETABLE_INDEXES);
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
	for(i=2; i<SIPCOMMONCFGEXPIRESSTATUSCODETABLE_INDEXES; i++) 
		c = c->next;
	col = 1;
	while(c) {
		if(col > SIPCOMMONCFGEXPIRESSTATUSCODETABLE_COLUMNS) {
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
	if(netsnmp_table_data_add_row(sipCommonCfgExpiresStatusCodeTable->table, row) != 
			SNMPERR_SUCCESS) {
		LOG(L_ERR, "%s: Error adding new row to table\n", func);
		netsnmp_table_data_delete_row(row);
		return -1;
	}

	return 0;
}

/* only accepts read-only data */
int sipCommonCfgExpiresStatusCodeTable_replaceRow(
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
	row = netsnmp_table_data_get(sipCommonCfgExpiresStatusCodeTable->table, idxs);
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
		if(col > SIPCOMMONCFGEXPIRESSTATUSCODETABLE_COLUMNS) {
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

/** handles requests for the sipCommonCfgExpiresStatusCodeTable table.
 * For every request it checks the specified object to see if it has a
 * handler, and calls it */
static int sipCommonCfgExpiresStatusCodeTable_handler(
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
		if(table_info->colnum > SIPCOMMONCFGEXPIRESSTATUSCODETABLE_COLUMNS)
			goto next;

		/* Get the handler and its object */
		if(sipCommonCfgExpiresStatusCodeTable_gh) {
			h = sipCommonCfgExpiresStatusCodeTable_gh;
			/* sip_obj is valid since we create upon registration */
			h->sip_obj->opaque = (void*)sipCommonCfgExpiresStatusCodeTable_replaceRow;
		} else {
			h = sipCommonCfgExpiresStatusCodeTable_h[table_info->colnum];
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
/** Initialize the sipCommonCfgExpiresMethodTable table by defining it's contents and 
 * how it's structured */
static int initialize_table_sipCommonCfgExpiresMethodTable()
{
	static oid sipCommonCfgExpiresMethodTable_oid[] = {1,3,6,1,2,1,9990,1,4,1};
	size_t sipCommonCfgExpiresMethodTable_oid_len = OID_LENGTH(sipCommonCfgExpiresMethodTable_oid);
	const char *func = "snmp_mod";

	/* create the table structure itself */
	sipCommonCfgExpiresMethodTable = netsnmp_create_table_data_set("sipCommonCfgExpiresMethodTable");
	if(!sipCommonCfgExpiresMethodTable) {
		LOG(L_ERR, "%s: Error creating table\n", func);
		return -1;
	}
	
	/***************************************************
	 * Adding indexes
	 */
	netsnmp_table_dataset_add_index(sipCommonCfgExpiresMethodTable, ASN_INTEGER);
	
	netsnmp_table_set_multi_add_default_row(
		sipCommonCfgExpiresMethodTable,
		COLUMN_SIPCFGEXPIRESINVITE, ASN_UNSIGNED, 1, NULL, 0,
		COLUMN_SIPCFGEXPIRESREGISTER, ASN_UNSIGNED, 1, NULL, 0,
		COLUMN_SIPCFGEXPIRESHEADERMETHOD, ASN_OCTET_STR, 1, NULL, 0,
		0);
	
	/* registering the table with the master agent */
	/* note: if you don't need a subhandler to deal with any aspects
	 * of the request, change sipCommonCfgExpiresMethodTable_handler to "NULL" */
	netsnmp_register_table_data_set(
			netsnmp_create_handler_registration(
				"sipCommonCfgExpiresMethodTable", 
				sipCommonCfgExpiresMethodTable_handler,
				sipCommonCfgExpiresMethodTable_oid,
				sipCommonCfgExpiresMethodTable_oid_len,
				HANDLER_CAN_RWRITE),
			sipCommonCfgExpiresMethodTable, NULL);

	return 0;
}

/* Initializes the handler table. Returns the registration function for this 
 * table */
reg_handler init_sipCommonCfgExpiresMethodTable_h()
{
	const char *func = "snmp_mod";
	sipCommonCfgExpiresMethodTable_h = calloc(SIPCOMMONCFGEXPIRESMETHODTABLE_COLUMNS+1,
		sizeof(struct sip_snmp_handler*));
	if(!sipCommonCfgExpiresMethodTable_h) {
		LOG(L_ERR, "%s: Error initializing handler table: %s\n", 
			func, strerror(errno));
		return NULL;
	}

	return sipCommonCfgExpiresMethodTable_reg;
}

/* Registration function. Called by snmp_register_handler() to register
 * objects belonging to this table. See snmp_handler.h for values of
 * op */
static int sipCommonCfgExpiresMethodTable_reg(struct sip_snmp_handler *h, int op)
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
		if(col <1 || col > SIPCOMMONCFGEXPIRESMETHODTABLE_COLUMNS) {
			LOG(L_ERR, "%s: attempt to register invalid column %d\n", 
				func, col);
			return -1;
		}

		/* add handler to table. We make copy to make everybody's life easier 
		 * (note that value is not copied) */
		if(!(sipCommonCfgExpiresMethodTable_h[col] = snmp_clone_handler(h))) {
			LOG(L_ERR, "%s: Error registering handler: %s\n", func,
				strerror(errno));
			return -1;
		}

		if(sipCommonCfgExpiresMethodTable_addObj(h, col) == -1) {
			LOG(L_ERR, "%s: Error adding new object to table\n", func);
			snmp_free_handler(sipCommonCfgExpiresMethodTable_h[col]);
			sipCommonCfgExpiresMethodTable_h[col] = NULL;
			return -1;
		}
	} else if(op == NEW_ROW) {
		if(!h->sip_obj || !h->sip_obj->value.voidp) {
			LOG(L_ERR, "%s: attemp to register invalid handler\n", func);
			return -1;
		}
		/* if there's global handler, no need to add the handlers */
		if(!sipCommonCfgExpiresMethodTable_gh && first) {
			/* first time around, jump over indexes, and copy the handlers.
			 * (Indexes don't have handlers associated with them). The default
			 * index (e.g. applIndex) shouldn't be passed in, that's why
			 * we jump over one less index. */
			c = h;
			if(SIPCOMMONCFGEXPIRESMETHODTABLE_INDEXES > 1) {
				for(i=1; i<SIPCOMMONCFGEXPIRESMETHODTABLE_INDEXES; i++)
					c = c->next;
				col = 2;	/* 2nd or 3rd index will be at column 1 */
			} else
				col = 1;
			while(c) {
				/* if too many we ignore the rest */
				if(col > SIPCOMMONCFGEXPIRESMETHODTABLE_COLUMNS) {
					LOG(L_ERR, "%s: Attempt to add too many objects to "
						"table row\n", func);
					return -1;
				}
				c->sip_obj->col = col;
				if(!(sipCommonCfgExpiresMethodTable_h[col] = snmp_clone_handler(c))) {
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
		if(sipCommonCfgExpiresMethodTable_newRow(h) == -1) {
			LOG(L_ERR, "%s: Error creating new table\n", func);
			return -1;
		}
	} else if(op == REG_TABLE) {
		if(!sipCommonCfgExpiresMethodTable_gh)
			sipCommonCfgExpiresMethodTable_gh = snmp_clone_handler(h);
		else
			memcpy(sipCommonCfgExpiresMethodTable_gh, h, sizeof(struct sip_snmp_handler));
		if(!sipCommonCfgExpiresMethodTable_gh) {
			LOG(L_ERR, "%s: Error registering table handler\n", func);
			return -1;
		}
		/* it's not necessary for the caller to create this, but we need
		 * it to pass info down to the handling function */
		if(!sipCommonCfgExpiresMethodTable_gh->sip_obj) {
			sipCommonCfgExpiresMethodTable_gh->sip_obj = calloc(1, 
					sizeof(struct sip_snmp_obj));
			if(!sipCommonCfgExpiresMethodTable_gh->sip_obj) {
				LOG(L_ERR, "%s: Error registering table handler\n", func);
				free(sipCommonCfgExpiresMethodTable_gh);
				sipCommonCfgExpiresMethodTable_gh = NULL;
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
static int sipCommonCfgExpiresMethodTable_addObj(struct sip_snmp_handler *h, int col)
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
	if(col < 1 || col > SIPCOMMONCFGEXPIRESMETHODTABLE_COLUMNS) {
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
		if(netsnmp_table_data_add_row(sipCommonCfgExpiresMethodTable->table, row) != 
				SNMPERR_SUCCESS) {
			LOG(L_ERR, "%s: Error adding new row to table\n",func); 
			return -1;
		}
	}

	/* Fin */
	return 0;
}

/* Handles creation of new rows for the table */
static int sipCommonCfgExpiresMethodTable_newRow(struct sip_snmp_handler *h)
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
	for(i=1; i<SIPCOMMONCFGEXPIRESMETHODTABLE_INDEXES; i++) {
		if(!c) {
			LOG(L_ERR, "%s: Not enought indexes passed, need %d\n", func,
				SIPCOMMONCFGEXPIRESMETHODTABLE_INDEXES);
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
	for(i=2; i<SIPCOMMONCFGEXPIRESMETHODTABLE_INDEXES; i++) 
		c = c->next;
	col = 1;
	while(c) {
		if(col > SIPCOMMONCFGEXPIRESMETHODTABLE_COLUMNS) {
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
	if(netsnmp_table_data_add_row(sipCommonCfgExpiresMethodTable->table, row) != 
			SNMPERR_SUCCESS) {
		LOG(L_ERR, "%s: Error adding new row to table\n", func);
		netsnmp_table_data_delete_row(row);
		return -1;
	}

	return 0;
}

/* only accepts read-only data */
int sipCommonCfgExpiresMethodTable_replaceRow(
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
	row = netsnmp_table_data_get(sipCommonCfgExpiresMethodTable->table, idxs);
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
		if(col > SIPCOMMONCFGEXPIRESMETHODTABLE_COLUMNS) {
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

/** handles requests for the sipCommonCfgExpiresMethodTable table.
 * For every request it checks the specified object to see if it has a
 * handler, and calls it */
static int sipCommonCfgExpiresMethodTable_handler(
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
		if(table_info->colnum > SIPCOMMONCFGEXPIRESMETHODTABLE_COLUMNS)
			goto next;

		/* Get the handler and its object */
		if(sipCommonCfgExpiresMethodTable_gh) {
			h = sipCommonCfgExpiresMethodTable_gh;
			/* sip_obj is valid since we create upon registration */
			h->sip_obj->opaque = (void*)sipCommonCfgExpiresMethodTable_replaceRow;
		} else {
			h = sipCommonCfgExpiresMethodTable_h[table_info->colnum];
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
