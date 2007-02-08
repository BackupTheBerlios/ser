#ifndef STACK_H_
#define STACK_H_

typedef struct {
	char  *buff;
	int    changed;
} stack_t;

#define NEW_STACK {NULL, 0}

void     stack_add_name(stack_t *stack, str *name);
void     stack_add_param(stack_t *stack, char *param);
char*    stack_print(stack_t *stack);
stack_t *stack_dup(stack_t *stack);
void     stack_free(stack_t *stack);
void     stack_clear(stack_t *stack);
inline int  stack_changed(stack_t *stack);

#endif /*STACK_H_*/
