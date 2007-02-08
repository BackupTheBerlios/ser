#ifndef _SELECT_H
#define _SELECT_H

#include "str.h"

#define MAX_SELECT_PARAMS 32
#define MAX_NESTED_CALLS  4

/* Flags for parser table FLAG bitfiels 
 */
#define DIVERSION_MASK   0x00FF

/* if DIVERSION is set and the function is accepted
 * the param is changed into SEL_PARAM_DIV and the value is set to (flags & DIVERSION_MASK)
 *  - it is valuable for STR params (saves parsing time)
 *  - does not release the memory occupied by the parameter
 */
#define DIVERSION        1<<8

/* set if any parameter is expected at this stage
 * (the function must be resolved further)
 */
#define SEL_PARAM_EXPECTED   1<<9

/* accept if following parameter is STR (any)
 * consume that extra parameter in one step
 */
#define CONSUME_NEXT_STR 1<<10

/* accept if following parameter is INT
 * consume that extra parameter in one ste
 */
#define CONSUME_NEXT_INT 1<<11

/* next parameter is optional (use with CONSUME_NEXT_STR or CONSUME_NEXT_INT
 * resolution is accepted even if there is no other parameter
 * or the parameter is of wrong type
 */
#define OPTIONAL         1<<12

/* left function is noted to be called
 * rigth function continues in resolution
 * NOTE: the parameter is not consumed for PARENT, 
 * so you can leave it as ..,SEL_PARAM_INT, 0,..
 *
 * run_select then calls all functions with PARENT flag
 * in the order of resolution until the final call or 
 * the result is != 0 (<0 error, 1 null str) 
 * the only one parameter passed between nested calls
 * is the result str*
 */
#define NESTED		1<<13

/* "fixup call" would be done, when the structure is resolved to this node
 * which means call with res and msg NULL
 *
 * if the fixup call return value <0, the select resolution will fail
 */
#define FIXUP_CALL	1<<14

/*
 * Selector call parameter
 */
typedef enum {
	SEL_PARAM_INT,  /* Integer parameter */
	SEL_PARAM_STR,  /* String parameter */
	SEL_PARAM_DIV,  /* Integer value got from parsing table */
	SEL_PARAM_PTR   /* void* data got from e.g. fixup call */
} select_param_type_t;
	
typedef union {
	int i;  /* Integer value */
	str s;  /* String value */
	void* p;/* Any data ptr */
} select_param_value_t;
	
typedef struct sel_param {
        select_param_type_t type;
        select_param_value_t v;
} select_param_t;

struct select;

typedef int (*select_f)(str* res, struct select* s, void* msg);

typedef struct select {
	select_f f[MAX_NESTED_CALLS];
	int param_offset[MAX_NESTED_CALLS+1];
	select_param_t params[MAX_SELECT_PARAMS];
	int n;
	int lvl;
} select_t;

typedef struct {
	select_f curr_f;
	select_param_type_t type;
	str name;
	select_f new_f;
	int flags;
} select_row_t;

typedef struct select_table {
  select_row_t *table;
  struct select_table *next;
} select_table_t;

int scan_select_table(select_row_t* table);

#endif /* _SELECT_H */
