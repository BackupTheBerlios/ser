/* 
 * $Id: c_elem.c,v 1.4 2002/03/05 14:36:03 janakj Exp $ 
 */

#include "c_elem.h"
#include <stdlib.h>
#include <stdio.h>   /* remove */
#include "utils.h"
#include "../../dprint.h"
#include "../../mem/mem.h"


/*
 * Create new element structure and initialize members
 */
c_elem_t* create_element(location_t* _loc)
{
	c_elem_t* ptr;

	ptr = (c_elem_t*)pkg_malloc(sizeof(c_elem_t));
	if (!ptr) {
		LOG(L_ERR, "create_element(): No memory left\n");
		return NULL;
	}

	ptr->ll.next = ptr->ll.prev = NULL;
	ptr->loc = _loc;
	ptr->ht_slot = NULL;
	ptr->c_ll.prev = ptr->c_ll.next = NULL;
	
	return ptr;
}


/*
 * Dispose an element
 * Must be removed from all linked lists !
 */
void free_element(c_elem_t* _el)
{
#ifdef PARANOID
	if (!_el) return;
#endif

	if (_el->loc) free_location(_el->loc);
	pkg_free(_el);
}




/*
 * Print an element, just for debugging purposes
 */
void print_element(c_elem_t* _el)
{
#ifdef PARANOID
	if (!_el) {
		LOG(L_ERR, "print_element(): Invalid _el parameter value\n");
		return;
	}
#endif

	printf("Nothing to print right now\n");
	printf("location:\n");
	print_location(_el->loc);
}
