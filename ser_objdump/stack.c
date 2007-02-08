#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "str.h"
#include "stack.h"

void stack_add_name(stack_t *stack, str *name)
{
	char *new_buff;
	char *str;
	char *aname;
	
	assert(stack);
	assert(name);
	
	str = "%s";
	aname = str;
	
	if (name->s != NULL)
		asprintf(&aname, "%*s", STR_FMT(name));
	
	if (stack->buff) {
		asprintf(&new_buff, "%s.%s", stack->buff, aname);
		free(stack->buff);
		stack->buff = new_buff;
	}
	else {
		asprintf(&(stack->buff), "%s", aname);
	}
	if (aname != str)
		free(aname);

	stack->changed = 1;
}
void stack_add_param(stack_t *stack, char *param)
{
	char *new_buff;
	
	assert(stack);
	assert(stack->buff);	/* never adds param into empty stack */
	assert(param);
	
	asprintf(&new_buff, "%s[%s]", stack->buff, param);
	free(stack->buff);
	stack->buff = new_buff;

	stack->changed = 1;
}

char* stack_print(stack_t *stack)
{
	char* buf;

	asprintf(&buf, "@%s", stack->buff);
	stack->changed = 0;
	return buf;
}

stack_t *stack_dup(stack_t *stack)
{
	stack_t *ns;
	
	ns = (stack_t *)malloc(sizeof(stack_t));

	if (!stack) {
		ns->buff = NULL;
		ns->changed = 0;
	}
	else {
		ns->buff = strdup(stack->buff);
		ns->changed = stack->changed;
	}
	return ns;
}

void stack_free(stack_t *stack)
{
	if (!stack)
		return;
	
	if (stack->buff)
		free(stack->buff);
	
	free(stack);
}

void stack_clear(stack_t *stack)
{
	if (!stack)
		return;
	
	if (stack->buff)
		free(stack->buff);

	stack->buff = NULL;
	stack->changed = 0;
}

inline int stack_changed(stack_t *stack)
{
	return stack->changed;
}
