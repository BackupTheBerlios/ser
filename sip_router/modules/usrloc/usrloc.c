/* $Id: usrloc.c,v 1.4 2002/02/24 17:09:10 janakj Exp $
 *
 * User location support
 *
 */


#include "usrloc.h"
#include "../../dprint.h"
#include "cache.h"
#include "utils.h"
#include "../../sr_module.h"
#include "to_parser.h"
#include "db.h"
#include "../../action.h"

#define TABLE_NAME "location"


#define HDR_FROM 10
#define HDR_CONTACT 11
#define HDR_EXPIRES 12

#define DB_URL "sql://janakj:heslo@localhost/ser"


static int save_contact(struct sip_msg*, char*, char*);
static int lookup_contact(struct sip_msg*, char*, char*);
static int rwrite(struct sip_msg* _msg, char* _c);

void destroy(void);


db_con_t* db_con;


static struct module_exports usrloc_exports= {	"usrloc", 
						(char*[]) {
							"save_contact",
							"lookup_contact"
						},
						(cmd_function[]) {
							save_contact, 
							lookup_contact
						},
						(int[]){1, 1},
						(fixup_function[]){0, 0},
						2,
						0,
						destroy,
						0 /* oncancel function */
};


static cache_t* c;


struct module_exports* mod_register()
{
	location_t* loc;
	printf( "Registering user location module\n");

	if (bind_dbmod()) {
		LOG(L_ERR, "mod_register(): Unable to bind database module\n");
	}

	db_con = db_init(DB_URL);
	if (!db_con) {
		LOG(L_ERR, "mod_register(): Unable to connect database\n");
	}

 	c = create_cache(512, db_con, TABLE_NAME);
	if (c == NULL) {
		LOG(L_ERR, "mod_register(): Unable to create cache\n");
	}
	preload_cache(c);
	return &usrloc_exports;
}


void destroy(void)
{
	free_cache(c);
	db_close(db_con);
}


static int send_200(struct sip_msg* _msg, location_t* _loc)
{
	printf("Sending 200 OK\n");
	return TRUE;
}


static int send_400(struct sip_msg* _msg)
{
	printf("Sending 400 Bad Request\n");
	return TRUE;
}


static int send_500(struct sip_msg* _msg)
{
	printf("Sendig 500 Server Error response\n");
	return TRUE;
}



static inline int process_star_loc(struct sip_msg* _msg, cache_t* _c, location_t* _loc)
{
	     /* Remove all bindings with the same address
	      * of record
	      */
	DBG("process_star_loc(): Removing all bindings from cache\n");
	if (cache_remove(_c, _loc->user.s) == FALSE) {
		LOG(L_ERR, "process_star_loc(): Error while removing cache entry\n");
		return FALSE;
	}
	
	DBG("process_star_loc(): Sending 200 OK\n");
	if (send_200(_msg, NULL) == FALSE) {
		LOG(L_ERR, "process_star_loc(): Error while sending 200 response\n");
		return FALSE;
	}
	
	return TRUE;
}


static inline int process_no_contacts(struct sip_msg* _msg, cache_t* _c, location_t* _loc)
{
	c_elem_t* el;

	el = cache_get(c, _loc->user.s);
	if (!el) {
		DBG("process_no_contacts(): No bindings found\n");
		DBG("process_no_contacts(): Sending 200 OK\n");
		if (send_200(_msg, NULL) == FALSE) {
			LOG(L_ERR, "process_loc(): Error while sending 200 response\n");
			return FALSE;
		}
		return TRUE;
	} else {
		DBG("process_no_contacts(): Bindings found\n");
		DBG("process_no_contacts(): Sending 200 OK with bindings\n");
		if (send_200(_msg, el->loc) == FALSE) {
			LOG(L_ERR, "process_loc(): Error while sending 200 response\n");
			cache_release_elem(el);
			return FALSE;
		}
		
		cache_release_elem(el);
		return TRUE;
	}
}


static inline int process_contacts(struct sip_msg* _msg, cache_t* _c, location_t* _loc)
{
	c_elem_t* el;

	el = cache_get(_c, _loc->user.s);
	if (el) {
		DBG("process_contacts(): Location found in cache\n");
		DBG("process_contacts(): Updating location\n");
		if (cache_update(_c, el, _loc) == FALSE) {
			LOG(L_ERR, "process_contacts(): Error while updating bindings in cache\n");
			cache_release_elem(el);
			return FALSE;
		}
		
		DBG("process_contacts(): Sending 200 OK\n");
		if (send_200(_msg, el->loc) == FALSE) {
			LOG(L_ERR, "process_contacts(): Error while sending 200 response\n");
			cache_release_elem(el);
			return FALSE;
		}

		cache_release_elem(el);
		return TRUE;
	} else {
		DBG("process_contacts(): Location not found in cache\n");
		DBG("process_contacts(): Inserting location into cache\n");
		if (remove_zero_expires(_loc) == FALSE) {
			LOG(L_ERR, "process_contacts(): Error while removing zero expires contacts\n");
			return FALSE;
		}
		if (cache_put(_c, _loc) == FALSE) {
			LOG(L_ERR, "process_contacts(): Error while inserting location\n");
			return FALSE;
		}
		DBG("process_contacts(): Sending 200 OK\n");
		if (send_200(_msg, _loc) == FALSE) {
			LOG(L_ERR, "process_contacts(): Error while sending 200 response\n");
			return FALSE;
		}
	}
}



static int process_loc(struct sip_msg* _msg, cache_t* _c, location_t* _loc, int star)
{
	if (star == 1) {
		DBG("process_loc(): star = 1, processing\n");
		return process_star_loc(_msg, _c, _loc);
	} else {
		if (!_loc->contacts) {
			DBG("process_loc(): No contacts found\n");
			return process_no_contacts(_msg, _c, _loc);
		} else {
			DBG("process_loc(): Contacts found\n");
			return process_contacts(_msg, _c, _loc);
		}
	}
}




static int save_contact(struct sip_msg* _msg, char* _table, char* _str2)
{
	location_t* loc;
	int star, expires, valid;

#ifdef PARANOID
	if (!_msg) return -1;
	if (!_table) return -2;
#endif
	if (msg2loc(_msg, &loc, &star, &expires) == FALSE) {
		LOG(L_ERR, "save_contact(): Unable to convert SIP message to location_t\n");
		return -1;
	}

	DBG("save_contact(): Validating request\n");
	if (validate_location(loc, expires, star, &valid) == FALSE) {
		LOG(L_ERR, "save_contact(): Error while validating request\n");
		free_location(loc);
		return -1;
	}

	if (!valid) {
		DBG("save_contact(): Request not validated, sending 400\n");
		if (send_400(_msg) == FALSE) {
			LOG(L_ERR, "save_contact(): Error while sending 400 response\n");
			free_location(loc);
			return -1;
		}
		free_location(loc);
		return 1;
	}
	DBG("save_contact(): Request validated, processing...\n");

	if (process_loc(_msg, c, loc, star) == FALSE) {
		LOG(L_ERR, "save_contact(): Error while processing location\n");
		free_location(loc);
		return -1;
	}

	print_cache(c);
	return 1;
}



/*
 * FIXME: Pouzit uz parsovanou To hlavicku
 */
static int lookup_contact(struct sip_msg* _msg, char* _table, char* _str2)
{
	location_t* loc;
	char* to_user;
	c_elem_t* el;
#ifdef PARANOID
	if (!_msg) return -1;
	if (!_table) return -2;
#endif
	
	if (!_msg->to) {
		if (parse_headers(_msg, HDR_TO) == -1) {
			LOG(L_ERR, "lookup_contact(): Error while parsing headers\n");
			return -1;
		}
	}

	if (!_msg->to) {
		LOG(L_ERR, "lookup_contact(): Unable to find To header field\n");
		return -1;
	}
	
	to_user = get_to_username(_msg->to->body.s, _msg->to->body.len);
	
	if (to_user == NULL) {
		LOG(L_ERR, "lookup_contact(): Unable to get user name\n");
		return -1;
	}
	
	el = cache_get(c, to_user);
	
	if (el) {
		if (el->loc->contacts) {
			if (rwrite(_msg, el->loc->contacts->c.s) == FALSE) {
				LOG(L_ERR, "lookup_contact(): Unable to rewrite message\n");
			}
		}
		cache_release_elem(el);
		return 1;
	} else {
		return 0;
	}
}



static int rwrite(struct sip_msg* _msg, char* _c)
{
	struct action act;

#ifdef PARANOID
	if (!_msg) return FALSE;
	if (!_c) return FALSE;
#endif
	printf("rwrite: %s\n", _c);
	act.type = SET_URI_T;
	act.p1_type = STRING_ST;
	act.p1.string = _c;
	act.next = NULL;

	if (do_action(&act, _msg) < 0) {
		LOG(L_ERR, "rwrite(): Error in do_action\n");
		return FALSE;
	}
	return TRUE;
}
