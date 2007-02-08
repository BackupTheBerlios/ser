#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "ser_objdump.h"
#include "str.h"
#include "stack.h"
#include "select.h"

select_row_t* table;
unsigned int table_n;

static void process_param(select_row_t*, stack_t*);
static void traverse(select_row_t*, stack_t*);

static str* relocate_str(str* src)
{
	static str s;
	
	s.len = src->len;
	s.s = (char*)relocate(src->s);
	return &s;
}


static int select_row_compare(const void *p1, const void *p2)
{
	/* typecast to int makes compiler happy
	 * subtraction of function pointers is not good idea in general
	 */
	return (int)((select_row_t *)p1)->curr_f - (int)((select_row_t *)p2)->curr_f;
}


/* works only on sorted array */
static select_row_t *find_first_curr_f(select_row_t *base, select_row_t *row)
{
	select_row_t *rptr, *rv;
	
	assert( row >= base );
	
	rptr = row;
	rv = row;
	while ( rptr != base) {
		rptr--;
		if (rptr->curr_f == row->curr_f)
			rv = rptr;
	}
	return rv;
}


static char *param_type_name(int flags)
{
	static char type_int[] = "%i";
	static char type_string[] = "%s";
	
	if (flags & CONSUME_NEXT_STR)
		return type_string;
	
	if (flags & CONSUME_NEXT_INT)
		return type_int;

	return "oops";
}


static void scan_select(select_row_t* row, stack_t *stack)
{
	/* if not NESTED add name to stack */
	if (!(row->flags & NESTED)) {
		stack_add_name(stack, relocate_str(&row->name));
	}
	
	/* if NESTED and CONSUME_NEXT_STR add name to stack */
	if (row->flags & NESTED && row->flags & CONSUME_NEXT_STR) {
		stack_add_name(stack, relocate_str(&row->name));
	}

	/* if not NESTED => check params */
	if (!(row->flags & NESTED)) {
		process_param(row, stack);
	}

	/* Continue traversing the tree */
	traverse(row, stack);
}


static void process_descendants(select_row_t *row, stack_t *stack)
{
	select_row_t key, *rptr, *first;
	stack_t *new_stack;

	key.curr_f = row->new_f;	/* we will search if there is any item with curr_f == item[i].new_f */ 
	rptr = bsearch(&key, table, table_n, sizeof(select_row_t), select_row_compare);
	if (rptr == NULL) {
		fprintf(stderr, "ERROR - descendant not found\n");
		exit(1);
	}
	first = find_first_curr_f(table, rptr);
	rptr = first;
	while (first->curr_f == rptr->curr_f) {
		new_stack = stack_dup(stack);
		scan_select(rptr, new_stack);
		stack_free(new_stack);
		rptr++;
	}
}


static int optional_terminal(select_row_t *row)
{
	select_row_t *rptr, key;
	
	if ( !(row->flags & SEL_PARAM_EXPECTED) ) {

		/* We must treat unresolvable selects as terminals here */
		if (row->new_f == NULL) {
			return 1;
		}

		key.curr_f = row->new_f;
		rptr = bsearch(&key, table, table_n, sizeof(select_row_t), select_row_compare);

		/* it is a optional terminal if FOUND */
		if (rptr)
			return 1;
	}
	return 0;
}


static int terminal(select_row_t *row)
{
	select_row_t *rptr, key;
	
	if (row->new_f == NULL) return 1;

	key.curr_f = row->new_f;
	rptr = bsearch(&key, table, table_n, sizeof(select_row_t), select_row_compare);
	
	/* it is a terminal if NOT found */
	if (!rptr)
		return 1;

	return 0;
}


static void traverse(select_row_t *row, stack_t *stack)
{
	char* buf;

	/* print if possible */
	if ((optional_terminal(row) && stack_changed(stack)) || terminal(row)) {
		res.name = stack_print(stack);
		print_res(&res);
		free(res.name);
	}


	/* We must treat unresolvable selects as terminals here */
	/* Unresolvable select (links to a function in the main SER binary) */
	if (row->new_f == NULL) {
		buf = stack_print(stack);
		fprintf(stderr, "WARNING: Selects starting with %s in module %s cannot be fully resolved\n", buf, res.module);
		free(buf);
	}

	/* process descendants
	 * for-each curr_f = new_f: scan_select
	 */
	if (!terminal(row)) {
		process_descendants(row, stack);
	}
}


static void process_param(select_row_t *row, stack_t *stack)
{
	if (row->flags & (CONSUME_NEXT_STR | CONSUME_NEXT_INT)) {
		if (row->flags & OPTIONAL) {
			traverse(row, stack);
		}

		/* Add the parameter to the stack */
		stack_add_param(stack, param_type_name(row->flags));
	}
}


int scan_select_table(select_row_t* sel)
{
	int i;
	stack_t stack = NEW_STACK;
	
	/* Scan the array to determine the number of rows */
	table = sel;
	for(table_n = 0; table[table_n].curr_f || table[table_n].new_f; table_n++);
	if (table_n == 0) {
		fprintf(stderr, "Empty select table found in module %s, this is weird\n", res.module);
		return -1;
	}
	
	qsort(table, table_n, sizeof(select_row_t), select_row_compare);
	
	for(i = 0; i < table_n; i++ ) {
		if (table[i].curr_f == NULL && table[i].new_f == NULL) {
			continue;
		}

		if (table[i].curr_f != NULL) {
			continue;
		}

		scan_select(&table[i], &stack);
		stack_clear(&stack);
	}

	return 0;
}
