/*
 * $Id: sipCommonStatusCode.c,v 1.2 2002/09/19 12:23:55 jku Rel $
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

/* Table sipStatusCodesTable */
static netsnmp_table_data_set* sipStatusCodesTable;

static int initialize_table_sipStatusCodesTable();
static Netsnmp_Node_Handler sipStatusCodesTable_handler;

/* The dynamic handler table */
static struct sip_snmp_handler** sipStatusCodesTable_h;
/* The global handler (if owner decides to handle this table that way) */
static struct sip_snmp_handler* sipStatusCodesTable_gh;
/* The registration function and friends */
static int sipStatusCodesTable_reg(struct sip_snmp_handler *h, int op);
static int sipStatusCodesTable_addObj(struct sip_snmp_handler *h, int col);
static int sipStatusCodesTable_newRow(struct sip_snmp_handler *h);

/* Table sipCommonStatusCodeTable */
static netsnmp_table_data_set* sipCommonStatusCodeTable;

static int initialize_table_sipCommonStatusCodeTable();
static Netsnmp_Node_Handler sipCommonStatusCodeTable_handler;

/* The dynamic handler table */
static struct sip_snmp_handler** sipCommonStatusCodeTable_h;
/* The global handler (if owner decides to handle this table that way) */
static struct sip_snmp_handler* sipCommonStatusCodeTable_gh;
/* The registration function and friends */
static int sipCommonStatusCodeTable_reg(struct sip_snmp_handler *h, int op);
static int sipCommonStatusCodeTable_addObj(struct sip_snmp_handler *h, int col);
static int sipCommonStatusCodeTable_newRow(struct sip_snmp_handler *h);

/** Initializes the sipCommonStatusCode module */
int init_sipCommonStatusCode()
{
	const char *func = "snmp_mod";
	/* here we initialize all the tables we're planning on supporting */
	if(initialize_table_sipStatusCodesTable() == -1) {
		LOG(L_ERR, "%s: Failed creating table sipStatusCodesTable\n", func);
		return -1;
	}
	if(initialize_table_sipCommonStatusCodeTable() == -1) {
		LOG(L_ERR, "%s: Failed creating table sipCommonStatusCodeTable\n", func);
		return -1;
	}
	return 0;
}

/** Initialize the sipStatusCodesTable table by defining it's contents and 
 * how it's structured */
static int initialize_table_sipStatusCodesTable()
{
	static oid sipStatusCodesTable_oid[] = {1,3,6,1,2,1,9990,1,7,1};
	size_t sipStatusCodesTable_oid_len = OID_LENGTH(sipStatusCodesTable_oid);
	const char *func = "snmp_mod";

	/* create the table structure itself */
	sipStatusCodesTable = netsnmp_create_table_data_set("sipStatusCodesTable");
	if(!sipStatusCodesTable) {
		LOG(L_ERR, "%s: Error creating table\n", func);
		return -1;
	}
	
	/***************************************************
	 * Adding indexes
	 */
	netsnmp_table_dataset_add_index(sipStatusCodesTable, ASN_INTEGER);
	
	netsnmp_table_set_multi_add_default_row(
		sipStatusCodesTable,
		COLUMN_SIPSTATSINFOCLASSINS, ASN_COUNTER, 0, NULL, 0,
		COLUMN_SIPSTATSINFOCLASSOUTS, ASN_COUNTER, 0, NULL, 0,
		COLUMN_SIPSTATSSUCCESSCLASSINS, ASN_COUNTER, 0, NULL, 0,
		COLUMN_SIPSTATSSUCCESSCLASSOUTS, ASN_COUNTER, 0, NULL, 0,
		COLUMN_SIPSTATSREDIRCLASSINS, ASN_COUNTER, 0, NULL, 0,
		COLUMN_SIPSTATSREDIRCLASSOUTS, ASN_COUNTER, 0, NULL, 0,
		COLUMN_SIPSTATSREQFAILCLASSINS, ASN_COUNTER, 0, NULL, 0,
		COLUMN_SIPSTATSREQFAILCLASSOUTS, ASN_COUNTER, 0, NULL, 0,
		COLUMN_SIPSTATSSERVERFAILCLASSINS, ASN_COUNTER, 0, NULL, 0,
		COLUMN_SIPSTATSSERVERFAILCLASSOUTS, ASN_COUNTER, 0, NULL, 0,
		COLUMN_SIPSTATSGLOBALFAILCLASSINS, ASN_COUNTER, 0, NULL, 0,
		COLUMN_SIPSTATSGLOBALFAILCLASSOUTS, ASN_COUNTER, 0, NULL, 0,
		COLUMN_SIPSTATSOTHERCLASSESINS, ASN_COUNTER, 0, NULL, 0,
		COLUMN_SIPSTATSOTHERCLASSESOUTS, ASN_COUNTER, 0, NULL, 0,
		0);
	
	/* registering the table with the master agent */
	/* note: if you don't need a subhandler to deal with any aspects
	 * of the request, change sipStatusCodesTable_handler to "NULL" */
	netsnmp_register_table_data_set(
			netsnmp_create_handler_registration(
				"sipStatusCodesTable", 
				sipStatusCodesTable_handler,
				sipStatusCodesTable_oid,
				sipStatusCodesTable_oid_len,
				HANDLER_CAN_RWRITE),
			sipStatusCodesTable, NULL);

	return 0;
}

/* Initializes the handler table. Returns the registration function for this 
 * table */
reg_handler init_sipStatusCodesTable_h()
{
	const char *func = "snmp_mod";
	sipStatusCodesTable_h = calloc(SIPSTATUSCODESTABLE_COLUMNS+1,
		sizeof(struct sip_snmp_handler*));
	if(!sipStatusCodesTable_h) {
		LOG(L_ERR, "%s: Error initializing handler table: %s\n", 
			func, strerror(errno));
		return NULL;
	}

	return sipStatusCodesTable_reg;
}

/* Registration function. Called by snmp_register_handler() to register
 * objects belonging to this table. See snmp_handler.h for values of
 * op */
static int sipStatusCodesTable_reg(struct sip_snmp_handler *h, int op)
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
		if(col <1 || col > SIPSTATUSCODESTABLE_COLUMNS) {
			LOG(L_ERR, "%s: attempt to register invalid column %d\n", 
				func, col);
			return -1;
		}

		/* add handler to table. We make copy to make everybody's life easier 
		 * (note that value is not copied) */
		if(!(sipStatusCodesTable_h[col] = snmp_clone_handler(h))) {
			LOG(L_ERR, "%s: Error registering handler: %s\n", func,
				strerror(errno));
			return -1;
		}

		if(sipStatusCodesTable_addObj(h, col) == -1) {
			LOG(L_ERR, "%s: Error adding new object to table\n", func);
			snmp_free_handler(sipStatusCodesTable_h[col]);
			sipStatusCodesTable_h[col] = NULL;
			return -1;
		}
	} else if(op == NEW_ROW) {
		if(!h->sip_obj || !h->sip_obj->value.voidp) {
			LOG(L_ERR, "%s: attemp to register invalid handler\n", func);
			return -1;
		}
		/* if there's global handler, no need to add the handlers */
		if(!sipStatusCodesTable_gh && first) {
			/* first time around, jump over indexes, and copy the handlers.
			 * (Indexes don't have handlers associated with them). The default
			 * index (e.g. applIndex) shouldn't be passed in, that's why
			 * we jump over one less index. */
			c = h;
			if(SIPSTATUSCODESTABLE_INDEXES > 1) {
				for(i=1; i<SIPSTATUSCODESTABLE_INDEXES; i++)
					c = c->next;
				col = 2;	/* 2nd or 3rd index will be at column 1 */
			} else
				col = 1;
			while(c) {
				/* if too many we ignore the rest */
				if(col > SIPSTATUSCODESTABLE_COLUMNS) {
					LOG(L_ERR, "%s: Attempt to add too many objects to "
						"table row\n", func);
					return -1;
				}
				c->sip_obj->col = col;
				if(!(sipStatusCodesTable_h[col] = snmp_clone_handler(c))) {
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
		if(sipStatusCodesTable_newRow(h) == -1) {
			LOG(L_ERR, "%s: Error creating new table\n", func);
			return -1;
		}
	} else if(op == REG_TABLE) {
		if(!sipStatusCodesTable_gh)
			sipStatusCodesTable_gh = snmp_clone_handler(h);
		else
			memcpy(sipStatusCodesTable_gh, h, sizeof(struct sip_snmp_handler));
		if(!sipStatusCodesTable_gh) {
			LOG(L_ERR, "%s: Error registering table handler\n", func);
			return -1;
		}
		/* it's not necessary for the caller to create this, but we need
		 * it to pass info down to the handling function */
		if(!sipStatusCodesTable_gh->sip_obj) {
			sipStatusCodesTable_gh->sip_obj = calloc(1, 
					sizeof(struct sip_snmp_obj));
			if(!sipStatusCodesTable_gh->sip_obj) {
				LOG(L_ERR, "%s: Error registering table handler\n", func);
				free(sipStatusCodesTable_gh);
				sipStatusCodesTable_gh = NULL;
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
static int sipStatusCodesTable_addObj(struct sip_snmp_handler *h, int col)
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
	if(col < 1 || col > SIPSTATUSCODESTABLE_COLUMNS) {
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
		if(netsnmp_table_data_add_row(sipStatusCodesTable->table, row) != 
				SNMPERR_SUCCESS) {
			LOG(L_ERR, "%s: Error adding new row to table\n",func); 
			return -1;
		}
	}

	/* Fin */
	return 0;
}

/* Handles creation of new rows for the table */
static int sipStatusCodesTable_newRow(struct sip_snmp_handler *h)
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
	for(i=1; i<SIPSTATUSCODESTABLE_INDEXES; i++) {
		if(!c) {
			LOG(L_ERR, "%s: Not enought indexes passed, need %d\n", func,
				SIPSTATUSCODESTABLE_INDEXES);
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
	for(i=2; i<SIPSTATUSCODESTABLE_INDEXES; i++) 
		c = c->next;
	col = 1;
	while(c) {
		if(col > SIPSTATUSCODESTABLE_COLUMNS) {
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
	if(netsnmp_table_data_add_row(sipStatusCodesTable->table, row) != 
			SNMPERR_SUCCESS) {
		LOG(L_ERR, "%s: Error adding new row to table\n", func);
		netsnmp_table_data_delete_row(row);
		return -1;
	}

	return 0;
}

/* only accepts read-only data */
int sipStatusCodesTable_replaceRow(
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
	row = netsnmp_table_data_get(sipStatusCodesTable->table, idxs);
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
		if(col > SIPSTATUSCODESTABLE_COLUMNS) {
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

/** handles requests for the sipStatusCodesTable table.
 * For every request it checks the specified object to see if it has a
 * handler, and calls it */
static int sipStatusCodesTable_handler(
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
		if(table_info->colnum > SIPSTATUSCODESTABLE_COLUMNS)
			goto next;

		/* Get the handler and its object */
		if(sipStatusCodesTable_gh) {
			h = sipStatusCodesTable_gh;
			/* sip_obj is valid since we create upon registration */
			h->sip_obj->opaque = (void*)sipStatusCodesTable_replaceRow;
		} else {
			h = sipStatusCodesTable_h[table_info->colnum];
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
/** Initialize the sipCommonStatusCodeTable table by defining it's contents and 
 * how it's structured */
static int initialize_table_sipCommonStatusCodeTable()
{
	static oid sipCommonStatusCodeTable_oid[] = {1,3,6,1,2,1,9990,1,7,2};
	size_t sipCommonStatusCodeTable_oid_len = OID_LENGTH(sipCommonStatusCodeTable_oid);
	const char *func = "snmp_mod";

	/* create the table structure itself */
	sipCommonStatusCodeTable = netsnmp_create_table_data_set("sipCommonStatusCodeTable");
	if(!sipCommonStatusCodeTable) {
		LOG(L_ERR, "%s: Error creating table\n", func);
		return -1;
	}
	
	/***************************************************
	 * Adding indexes
	 */
	netsnmp_table_dataset_add_index(sipCommonStatusCodeTable, ASN_INTEGER);
	netsnmp_table_dataset_add_index(sipCommonStatusCodeTable, ASN_INTEGER);
	
	netsnmp_table_set_multi_add_default_row(
		sipCommonStatusCodeTable,
		COLUMN_SIPCOMMONSTATUSCODEVALUE, ASN_INTEGER, 0, NULL, 0,
		COLUMN_SIPCOMMONSTATUSCODEINS, ASN_COUNTER, 0, NULL, 0,
		COLUMN_SIPCOMMONSTATUSCODEOUTS, ASN_COUNTER, 0, NULL, 0,
		COLUMN_SIPCOMMONSTATUSCODESTATUS, ASN_INTEGER, 1, NULL, 0,
		0);
	
	/* registering the table with the master agent */
	/* note: if you don't need a subhandler to deal with any aspects
	 * of the request, change sipCommonStatusCodeTable_handler to "NULL" */
	netsnmp_register_table_data_set(
			netsnmp_create_handler_registration(
				"sipCommonStatusCodeTable", 
				sipCommonStatusCodeTable_handler,
				sipCommonStatusCodeTable_oid,
				sipCommonStatusCodeTable_oid_len,
				HANDLER_CAN_RWRITE),
			sipCommonStatusCodeTable, NULL);

	return 0;
}

/* Initializes the handler table. Returns the registration function for this 
 * table */
reg_handler init_sipCommonStatusCodeTable_h()
{
	const char *func = "snmp_mod";
	sipCommonStatusCodeTable_h = calloc(SIPCOMMONSTATUSCODETABLE_COLUMNS+1,
		sizeof(struct sip_snmp_handler*));
	if(!sipCommonStatusCodeTable_h) {
		LOG(L_ERR, "%s: Error initializing handler table: %s\n", 
			func, strerror(errno));
		return NULL;
	}

	return sipCommonStatusCodeTable_reg;
}

/* Registration function. Called by snmp_register_handler() to register
 * objects belonging to this table. See snmp_handler.h for values of
 * op */
static int sipCommonStatusCodeTable_reg(struct sip_snmp_handler *h, int op)
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
		if(col <1 || col > SIPCOMMONSTATUSCODETABLE_COLUMNS) {
			LOG(L_ERR, "%s: attempt to register invalid column %d\n", 
				func, col);
			return -1;
		}

		/* add handler to table. We make copy to make everybody's life easier 
		 * (note that value is not copied) */
		if(!(sipCommonStatusCodeTable_h[col] = snmp_clone_handler(h))) {
			LOG(L_ERR, "%s: Error registering handler: %s\n", func,
				strerror(errno));
			return -1;
		}

		if(sipCommonStatusCodeTable_addObj(h, col) == -1) {
			LOG(L_ERR, "%s: Error adding new object to table\n", func);
			snmp_free_handler(sipCommonStatusCodeTable_h[col]);
			sipCommonStatusCodeTable_h[col] = NULL;
			return -1;
		}
	} else if(op == NEW_ROW) {
		if(!h->sip_obj || !h->sip_obj->value.voidp) {
			LOG(L_ERR, "%s: attemp to register invalid handler\n", func);
			return -1;
		}
		/* if there's global handler, no need to add the handlers */
		if(!sipCommonStatusCodeTable_gh && first) {
			/* first time around, jump over indexes, and copy the handlers.
			 * (Indexes don't have handlers associated with them). The default
			 * index (e.g. applIndex) shouldn't be passed in, that's why
			 * we jump over one less index. */
			c = h;
			if(SIPCOMMONSTATUSCODETABLE_INDEXES > 1) {
				for(i=1; i<SIPCOMMONSTATUSCODETABLE_INDEXES; i++)
					c = c->next;
				col = 2;	/* 2nd or 3rd index will be at column 1 */
			} else
				col = 1;
			while(c) {
				/* if too many we ignore the rest */
				if(col > SIPCOMMONSTATUSCODETABLE_COLUMNS) {
					LOG(L_ERR, "%s: Attempt to add too many objects to "
						"table row\n", func);
					return -1;
				}
				c->sip_obj->col = col;
				if(!(sipCommonStatusCodeTable_h[col] = snmp_clone_handler(c))) {
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
		if(sipCommonStatusCodeTable_newRow(h) == -1) {
			LOG(L_ERR, "%s: Error creating new table\n", func);
			return -1;
		}
	} else if(op == REG_TABLE) {
		if(!sipCommonStatusCodeTable_gh)
			sipCommonStatusCodeTable_gh = snmp_clone_handler(h);
		else
			memcpy(sipCommonStatusCodeTable_gh, h, sizeof(struct sip_snmp_handler));
		if(!sipCommonStatusCodeTable_gh) {
			LOG(L_ERR, "%s: Error registering table handler\n", func);
			return -1;
		}
		/* it's not necessary for the caller to create this, but we need
		 * it to pass info down to the handling function */
		if(!sipCommonStatusCodeTable_gh->sip_obj) {
			sipCommonStatusCodeTable_gh->sip_obj = calloc(1, 
					sizeof(struct sip_snmp_obj));
			if(!sipCommonStatusCodeTable_gh->sip_obj) {
				LOG(L_ERR, "%s: Error registering table handler\n", func);
				free(sipCommonStatusCodeTable_gh);
				sipCommonStatusCodeTable_gh = NULL;
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
static int sipCommonStatusCodeTable_addObj(struct sip_snmp_handler *h, int col)
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
	if(col < 1 || col > SIPCOMMONSTATUSCODETABLE_COLUMNS) {
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
		if(netsnmp_table_data_add_row(sipCommonStatusCodeTable->table, row) != 
				SNMPERR_SUCCESS) {
			LOG(L_ERR, "%s: Error adding new row to table\n",func); 
			return -1;
		}
	}

	/* Fin */
	return 0;
}

/* Handles creation of new rows for the table */
static int sipCommonStatusCodeTable_newRow(struct sip_snmp_handler *h)
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
	for(i=1; i<SIPCOMMONSTATUSCODETABLE_INDEXES; i++) {
		if(!c) {
			LOG(L_ERR, "%s: Not enought indexes passed, need %d\n", func,
				SIPCOMMONSTATUSCODETABLE_INDEXES);
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
	for(i=2; i<SIPCOMMONSTATUSCODETABLE_INDEXES; i++) 
		c = c->next;
	col = 1;
	while(c) {
		if(col > SIPCOMMONSTATUSCODETABLE_COLUMNS) {
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
	if(netsnmp_table_data_add_row(sipCommonStatusCodeTable->table, row) != 
			SNMPERR_SUCCESS) {
		LOG(L_ERR, "%s: Error adding new row to table\n", func);
		netsnmp_table_data_delete_row(row);
		return -1;
	}

	return 0;
}

/* only accepts read-only data */
int sipCommonStatusCodeTable_replaceRow(
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
	row = netsnmp_table_data_get(sipCommonStatusCodeTable->table, idxs);
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
		if(col > SIPCOMMONSTATUSCODETABLE_COLUMNS) {
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

/** handles requests for the sipCommonStatusCodeTable table.
 * For every request it checks the specified object to see if it has a
 * handler, and calls it */
static int sipCommonStatusCodeTable_handler(
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
		if(table_info->colnum > SIPCOMMONSTATUSCODETABLE_COLUMNS)
			goto next;

		/* Get the handler and its object */
		if(sipCommonStatusCodeTable_gh) {
			h = sipCommonStatusCodeTable_gh;
			/* sip_obj is valid since we create upon registration */
			h->sip_obj->opaque = (void*)sipCommonStatusCodeTable_replaceRow;
		} else {
			h = sipCommonStatusCodeTable_h[table_info->colnum];
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
