/* 
 * $Id: domain.c,v 1.26 2007/04/18 13:08:46 janakj Exp $
 *
 * Domain table related functions
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
 */

#include <string.h>
#include "domain_mod.h"
#include "../../dprint.h"
#include "../../mem/shm_mem.h"
#include "../../db/db.h"
#include "../../ut.h"


/*
 * Search the list of domains for domain with given did
 */
static domain_t* domain_search(domain_t* list, str* did)
{
	while(list) {
		if (list->did.len == did->len &&
		    !memcmp(list->did.s, did->s, did->len)) {
			return list;
		}
		list = list->next;
	}
	return 0;
}


/*
 * Add a new domain name to did
 */
static int domain_add(domain_t* d, str* domain, unsigned int flags)
{
	str* p1;
	unsigned int* p2;
	str dom;

	if (!d || !domain) {
		LOG(L_ERR, "domain:domain_add: Invalid parameter value\n");
		return -1;
	}

	dom.s = shm_malloc(domain->len);
	if (!dom.s) goto error;
	memcpy(dom.s, domain->s, domain->len);
	dom.len = domain->len;
	strlower(&dom);

	p1 = (str*)shm_realloc(d->domain, sizeof(str) * (d->n + 1));
	if (!p1) goto error;
	p2 = (unsigned int*)shm_realloc(d->flags, sizeof(unsigned int) * (d->n + 1));
	if (!p2) goto error;
	
	d->domain = p1;
	d->domain[d->n] = dom;
	d->flags = p2;
	d->flags[d->n] = flags;
	d->n++;
	return 0;

 error:
	LOG(L_ERR, "domain:domain_add: Unable to add new domain name (out of memory)\n");
	if (dom.s) shm_free(dom.s);
	return -1;
}


/*
 * Release all memory allocated for given domain structure
 */
static void free_domain(domain_t* d)
{
	int i;
	if (!d) return;
	if (d->did.s) shm_free(d->did.s);

	for(i = 0; i < d->n; i++) {
		if (d->domain[i].s) shm_free(d->domain[i].s);
	}
	shm_free(d->domain);
	shm_free(d->flags);
	if (d->attrs) destroy_avp_list(&d->attrs);
	shm_free(d);
}




/*
 * Create a new domain structure which will initialy have
 * one domain name
 */
static domain_t* new_domain(str* did, str* domain, unsigned int flags)
{
	domain_t* d;
	int_str name, val;
	str name_s = STR_STATIC_INIT(AVP_DID);

	d = (domain_t*)shm_malloc(sizeof(domain_t));
	if (!d) goto error;
	memset(d, 0, sizeof(domain_t));
	d->did.s = shm_malloc(did->len);
	if (!d->did.s) goto error;
	memcpy(d->did.s, did->s, did->len);
	d->did.len = did->len;

	d->domain = (str*)shm_malloc(sizeof(str));
	if (!d->domain) goto error;
	d->domain[0].s = shm_malloc(domain->len);
	if (!d->domain[0].s) goto error;
	memcpy(d->domain[0].s, domain->s, domain->len);
	d->domain[0].len = domain->len;
	strlower(d->domain);
	
	d->flags = (unsigned int*)shm_malloc(sizeof(unsigned int));
	if (!d->flags) goto error;
	d->flags[0] = flags;
	d->n = 1;

	     /* Create an attribute containing did of the domain */
	name.s = name_s;
	val.s = *did;
	if (add_avp_list(&d->attrs, AVP_CLASS_DOMAIN | AVP_NAME_STR | AVP_VAL_STR, name, val) < 0) goto error;

	return d;

 error:
	LOG(L_ERR, "domain:new_domain: Unable to create new domain structure\n");
	free_domain(d);
	return 0;
}


/*
 * Release all memory allocated for entire domain list
 */
void free_domain_list(domain_t* list)
{
	domain_t* ptr;
	if (!list) return;

	while(list) {
		ptr = list;
		list = list->next;
		free_domain(ptr);
	}
}


/*
 * Load attributes from domain_attrs table
 */
int db_load_domain_attrs(domain_t* d)
{
    int_str name, v;
	str avp_val;
    db_res_t* res;
	db_rec_t* rec;
    unsigned short flags;
    
	load_attrs_cmd->params[0].v.lstr = d->did;

	if (db_exec(&res, load_attrs_cmd) < 0) {
		ERR("Error while quering database\n");
		return -1;
    }
    
	rec = db_first(res);
	while(rec) {
		if (rec->fld[0].flags & DB_NULL ||
			rec->fld[1].flags & DB_NULL ||
			rec->fld[3].flags & DB_NULL) {
			ERR("Skipping row containing NULL entries\n");
			goto skip;
		}
		
		if ((rec->fld[3].v.int4 & DB_LOAD_SER) == 0) goto skip;
	
		/* Get AVP name */
		name.s = rec->fld[0].v.lstr;
		
		/* Test for NULL value */
		if (rec->fld[2].flags & DB_NULL) {
			avp_val.s = 0;
			avp_val.len = 0;
		} else {
			avp_val = rec->fld[2].v.lstr;
		}
		
		flags = AVP_CLASS_DOMAIN | AVP_NAME_STR;
		if (rec->fld[1].v.int4 == AVP_VAL_STR) {
			/* String AVP */
			v.s = avp_val;
			flags |= AVP_VAL_STR;
		} else {
			/* Integer AVP */
			str2int(&avp_val, (unsigned*)&v.n);
		}
		
		if (add_avp_list(&d->attrs, flags, name, v) < 0) {
			LOG(L_ERR, "domain:db_load_domain_attrs: Error while adding domain attribute %.*s to domain %.*s, skipping\n",
				name.s.len, ZSW(name.s.s),
				d->did.len, ZSW(d->did.s));
		}
		
	skip:
		rec = db_next(res);
    }
    db_res_free(res);
    return 0;
}


/*
 * Create domain list from domain table
 */
int load_domains(domain_t** dest)
{
	db_res_t* res = NULL;
	db_rec_t* rec;
	unsigned int flags;
	domain_t* d, *list;

	list = 0;

	if (db_exec(&res, load_domains_cmd) < 0) {
		ERR("Error while querying database\n");
		return -1;
	}

	rec = db_first(res);

	while(rec) {
		     /* Do not assume that the database server performs any constrain
		      * checking (dbtext does not) and perform sanity checks here to
		      * make sure that we only load good entried
		      */
		if (rec->fld[0].flags & DB_NULL || 
			rec->fld[1].flags & DB_NULL || 
			rec->fld[2].flags & DB_NULL) {
			ERR("Row with NULL column(s), skipping\n");
			goto skip;
		}

		flags = rec->fld[2].v.int4;
		
		/* Skip entries that are disabled/scheduled for removal */
		if (flags & DB_DISABLED) goto skip;
		     /* Skip entries that are for serweb/ser-ctl only */
		if (!(flags & DB_LOAD_SER)) goto skip;
		
		DBG("domain:load_domains: Processing entry (%.*s, %.*s, %u)\n",
		    rec->fld[0].v.lstr.len, ZSW(rec->fld[0].v.lstr.s),
		    rec->fld[1].v.lstr.len, ZSW(rec->fld[1].v.lstr.s),
		    flags);

		d = domain_search(list, &rec->fld[0].v.lstr);
		if (d) {
			/* DID exists in the list, update it */
			if (domain_add(d, &rec->fld[1].v.lstr, flags) < 0) goto error;
		} else {
			     /* DID does not exist yet, create a new entry */
			d = new_domain(&rec->fld[0].v.lstr, &rec->fld[1].v.lstr, flags);
			if (!d) goto error;
			d->next = list;
			list = d;
		}

	skip:
		rec = db_next(res);
	}

	db_res_free(res);

	if (load_domain_attrs) {
		d = list;
		while(d) {
			if (db_load_domain_attrs(d) < 0) goto error;
			d = d->next;
		}
	}

	*dest = list;
	return 0;

 error:
	if (res) db_res_free(res);
	free_domain_list(list);
	return 1;
}
