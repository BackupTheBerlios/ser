/*
 * $Id: test_handler.c,v 1.2 2002/09/19 12:23:55 jku Rel $
 *
 * SNMP Module
 * 
 * Example of a global table handler
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


#include "../../sr_module.h"
#include "snmp_handler.h"
#include <stdlib.h>
#include <errno.h>

int handle_transTable(struct sip_snmp_obj*, enum handler_op);

#define NUM_TRANS 10
int test_globalHandler()
{
	struct sip_snmp_handler *h;
	int (*reg)(const char *, struct sip_snmp_handler*);
	const char *func = __FUNCTION__;
	int i;

	h = snmp_new_handler(sizeof(unsigned long));
	if(!h) {
		LOG(L_ERR, "%s: %s\n", func, strerror(errno));
		return -1;
	}
	h->on_get = handle_transTable;

	reg = find_export("snmp_register_table", 2);
	if(!reg) {
		LOG(L_ERR, "%s: couldn't find registration function!!\n", func);
		return -1;
	}

	if(reg("sipTransactionTable", h) == -1) {
		LOG(L_ERR, "%s: couldn't register sipTransactionTable!!\n", func);
		return -1;
	}

	/* now add the rows. We just need to pass indexes for the transactions */
	reg = find_export("snmp_register_row", 2);
	if(!reg) {
		LOG(L_ERR, "%s: couldn't find registration function!!\n", func);
		return -1;
	}

	h->sip_obj->val_len = sizeof(unsigned long);
	h->sip_obj->type = SER_UNSIGNED;
	h->sip_obj->next = NULL;
	for(i=1; i<=NUM_TRANS; i++) {
		*h->sip_obj->value.integer = i;
		if(reg("sipTransactionTable", h) == -1) {
			LOG(L_ERR, "%s: Failed creating row %d\n", func, i);
			snmp_free_handler(h->sip_obj->value.voidp);
			return -1;
		}
	}

	snmp_free_handler(h);

	return 0;
}

/* o->opaque has function to fill up the desired row */
int handle_transTable(struct sip_snmp_obj* o, enum handler_op op)
{
	const char *func = __FUNCTION__;
	int (*fill)(struct sip_snmp_obj *, struct sip_snmp_obj *);
	struct sip_snmp_obj *idx, *data;
	char *to = "ricardo@iptel.org";

	if(o->col != 1)
		return 1;

	if(!o->opaque) {
		LOG(L_WARN, "%s: Invalid function to fill row!!\n", func);
		return -1;
	}
	fill = o->opaque;

	/* fill index. In this case, the index is the passed row */
	idx = snmp_new_obj(SER_INTEGER, &o->row, sizeof(unsigned int));
	if(!idx) {
		LOG(L_ERR, "%s: Error creating index object\n", func);
		return -1;
	}

	data = snmp_new_obj(SER_STRING, to, strlen(to));
	if(!data) {
		LOG(L_ERR, "%s: Error creating data for row\n", func);
		snmp_free_obj(idx);
		return -1;
	}

	if(fill(idx, data) == -1) {
		LOG(L_ERR, "%s: Couldn't put data into row\n", func);
		snmp_free_obj(idx);
		snmp_free_obj(data);
		return -1;
	}

	snmp_free_obj(idx);
	snmp_free_obj(data);
	
	return 1;
}
