/* 
 * $Id: cache.h,v 1.8 2002/04/12 00:36:01 janakj Exp $ 
 */

#ifndef CACHE_H
#define CACHE_H

#include "c_slot.h"
#include "c_elem.h"
#include "location.h"
#include "db.h"
#include "../../fastlock.h"
#include "../../str.h"


struct c_slot;
struct c_elem;


/*
 * Hash table
 */
typedef struct cache {
	int size;                     /* Hash table size */
	char* db_table;               /* Table in database */
	struct c_slot* table;         /* Hash table */
	struct {                      /* Linked list of all elements in the cache */
		int count;            /* Number of element in the linked list */
		struct c_elem* first; /* First element in the list */
		struct c_elem* last;  /* Last element in the list */
	} c_ll;
	fl_lock_t lock;               /* cache lock */
} cache_t;


/* Cache hash table size */
#define CACHE_SIZE(cache) ((cache)->size)

/* Get database table name associated with this cache */
#define CACHE_DB_TABLE(cache) ((cache)->db_table)

/* Get first element in the hash table */
#define CACHE_FIRST_ELEM(cache) ((cache)->c_ll.first)

/* Get number of elements in the hash table */
#define CACHE_ELEM_COUNT(cache) ((cache)->c_ll.count)

/* Get last elements in the hash table */
#define CACHE_LAST_ELEM(cache) ((cache)->c_ll.last)

/* Get a collision slot in hash table */
#define CACHE_GET_SLOT(cache,id) (&((cache)->table[id]))

/* Get cache lock */
#define CACHE_LOCK(cache) ((cache)->lock)

/* Get cache hash table */
#define CACHE_HTABLE(cache) ((cache)->table)

/*
 * Create a new cache structure
 */
cache_t* create_cache(int _size, const char* _table);


/*
 * Free all memory associated with
 * the given cache
 */
void free_cache(cache_t* _c);


/*
 * Insert a new location into cache
 */
int cache_insert(cache_t* _c, db_con_t* _con, location_t* _l);


/*
 * Get an element from cache
 */
struct c_elem* cache_get(cache_t* _c, str* _aor);


/*
 * Update a location in cache
 */
int cache_update_unsafe(cache_t* _c, db_con_t* _con, struct c_elem** _el, location_t* _loc, int* _sr);


/*
 * Remove a location from cache
 */
int cache_remove(cache_t* _c, db_con_t* _con, str* _aor);


/*
 * Mark element as released (can be modified
 * if neccessary)
 */
void cache_release_elem(struct c_elem* _el);

/*
 * Print cache content (for debugging
 * purpose only
 */
void print_cache(cache_t* _c);


/*
 * Preload cache content from database
 */
int preload_cache(cache_t* _c, db_con_t* _con);


/*
 * Remove expired bindings from cache
 */
int clean_cache(cache_t* _c, db_con_t* _con);


#endif
