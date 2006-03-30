/*
 *
 *  simple atomic ops testing program
 *  (no paralel stuff, just see if the opcodes are "legal")
 *
 *  Defines: TYPE - not defined => use atomic_t and the corresponding
 *                  atomic functions
 *                - long => use volatile long* and  the atomic_*_long functions
 *                - int  => use volatile int* and the atomic_*_int functions
 *           MEMBAR - if defined use mb_atomic_* instead of atomic_*
 *           NOSMP - use non smp versions
 *           NOASM - don't use asm inline version
 *           __CPU_xxx - use __CPU_xxx code
 *           SPARC64_MODE - compile for a sparc 64 in 64 bit mode (gcc -m64
 *                          must be used on solaris in this case)
 *  Example:  
 *    gcc -Wall -O3 -D__CPU_i386 -DNOSMP -DMEMBAR -DTYPE=long atomic_test2.c
 * 
 *  Compile with: gcc -Wall -O3 -D__CPU_i386  ... on x86 machines
 *                gcc -Wall -O3 -D__CPU_x86_64 ... on amd64 machines
 *                gcc -mips2 -Wall -O2 -D__CPU_mips2  ... on mips machines
 *                gcc -m64 -Wall -O2 -D__CPU_mips64 ... on mips64 machines
 *                gcc -O3 -Wall -D__CPU_ppc ... on powerpc machines
 *                gcc -m64 -O3 -Wall -D__CPU_ppc64 ... on powerpc machines
 *                gcc -m64 -O3 -Wall -D__CPU_sparc64 -DSPARC64_MODE ... on 
 *                                                   ultrasparc machines
 *                gcc -mcpu=v9 -O3 -Wall -D__CPU_sparc64  ... for 32 bit code 
 *                                                   (sparc32plus) on 
 *                                                   ultrasparc machines
 *                gcc -O3 -Wall -D__CPU_sparc ... on sparc v8 machines
 *  -- andrei
 *
 *  
 */

#include <stdio.h>

#ifndef NOASM
#define CC_GCC_LIKE_ASM
#endif

#include "../atomic_ops.h"

#ifdef ATOMIC_OPS_USE_LOCK 
/* hack to make lock work */
#include "../lock_ops.h"

gen_lock_t* _atomic_lock;

gen_lock_t dummy_lock;

#endif



#if defined MB || defined MEMBAR
#undef MB
#define MB mb_
#define MEMBAR_STR "membar "
#else
#define MB  /* empty */
#define MEMBAR_STR ""
#endif

#ifndef TYPE
#define SUF
#define ATOMIC_TYPE atomic_t
#define VALUE_TYPE volatile int
#define get_val(v)	(v->val)
#else
#define _SUF(T) _##T
#define _SUF1(T) _SUF(T)
#define SUF _SUF1(TYPE)
#define ATOMIC_TYPE volatile TYPE
#define VALUE_TYPE ATOMIC_TYPE
#define get_val(v)	(*v)
#endif


#define _STR(S) #S
#define STR(S) _STR(S)

static char* flags=
#ifdef NOASM
	"no_inline_asm "
#endif
#ifdef NOSMP
	"nosmp "
#else
	"smp "
#endif
	MEMBAR_STR
#ifndef HAVE_ASM_INLINE_MEMBAR
	"no_asm_membar(slow) "
#endif
#ifndef HAVE_ASM_INLINE_ATOMIC_OPS
	"no_asm_atomic_ops "
#endif
#ifdef TYPE
	STR(TYPE) " "
#else
	"atomic_t "
#endif
;



/* macros for atomic_* functions */

#define _AT_DECL(OP, P, S) \
	P##atomic_##OP##S


/* to make sure all the macro passed as params are expanded,
 *  go through a 2 level deep macro decl. */
#define _AT_DECL1(OP, P, S) _AT_DECL(OP, P, S)
#define AT_DECL(OP) _AT_DECL1(OP, MB, SUF)


#define at_set	AT_DECL(set)
#define at_get	AT_DECL(get)

#define at_inc	AT_DECL(inc)
#define at_dec	AT_DECL(dec)
#define at_inc_and_test	AT_DECL(inc_and_test)
#define at_dec_and_test	AT_DECL(dec_and_test)
#define at_and	AT_DECL(and)
#define at_or	AT_DECL(or)
#define at_get_and_set	AT_DECL(get_and_set)


#define CHECK_ERR(txt, x, y) \
	if (x!=y) { \
		fprintf(stderr, "ERROR: line %d: %s failed: expected 0x%02x but got "\
						"0x%02x.\n", \
						__LINE__, #txt, (unsigned) x, (unsigned) y);\
		goto error; \
	}

#define VERIFY(ops, y) \
	ops ; \
	CHECK_ERR( ops, get_val(v), y)


int main(int argc, char** argv)
{
	ATOMIC_TYPE var;
	VALUE_TYPE r;
	
	ATOMIC_TYPE* v;
	
	v=&var;
	
	
#ifdef ATOMIC_OPS_USE_LOCK
	/* init the lock (emulate atomic_ops.c) */
	_atomic_lock=&dummy_lock;
	if (lock_init(_atomic_lock)==0){
		fprintf(stderr, "ERROR: failed to initialize the lock\n");
		_atomic_lock=0;
		goto error;
	}
#endif
	
	printf("%s\n", flags);
	
	printf("starting memory barrier opcode tests...\n");
	membar();
	printf(" membar() .............................. ok\n");
	membar_write();
	printf(" membar_write() ........................ ok\n");
	membar_read();
	printf(" membar_read() ......................... ok\n");
	
	printf("\nstarting atomic ops basic tests...\n");
	
	VERIFY(at_set(v, 1), 1);
	printf(" atomic_set, v should be 1 ............. %2d\n", (int)at_get(v));
	VERIFY(at_inc(v), 2);
	printf(" atomic_inc, v should be 2 ............. %2d\n", (int)at_get(v));
	VERIFY(r=at_inc_and_test(v), 3);
	printf(" atomic_inc_and_test, v should be  3 ... %2d\n", (int)at_get(v));
	printf("                      r should be  0 ... %2d\n", (int)r);
	
	VERIFY(at_dec(v), 2);
	printf(" atomic_dec, v should be 2 ............. %2d\n", (int)at_get(v));
	VERIFY(r=at_dec_and_test(v), 1);
	printf(" atomic_dec_and_test, v should be  1 ... %2d\n", (int)at_get(v));
	printf("                      r should be  0 ... %2d\n", (int)r);
	VERIFY(r=at_dec_and_test(v), 0);
	printf(" atomic_dec_and_test, v should be  0 ... %2d\n", (int)at_get(v));
	printf("                      r should be  1 ... %2d\n", (int)r);
	VERIFY(r=at_dec_and_test(v), -1);
	printf(" atomic_dec_and_test, v should be -1 ... %2d\n", (int)at_get(v));
	printf("                      r should be  0 ... %2d\n", (int)r);
	
	VERIFY(at_and(v, 2), 2);
	printf(" atomic_and, v should be 2 ............. %2d\n", (int)at_get(v));
	
	VERIFY(at_or(v, 5), 7);
	VERIFY(r=at_get_and_set(v, 0), 0);
	printf(" atomic_or,  v should be 7 ............. %2d\n", (int)r);
	printf(" atomic_get_and_set, v should be 0 ..... %2d\n", (int)at_get(v));

	
	printf("\ndone.\n");
#ifdef ATOMIC_OPS_USE_LOCK
	lock_destroy(_atomic_lock);
#endif
	return 0;
error:
#ifdef ATOMIC_OPS_USE_LOCK
	if (_atomic_lock)
		lock_destroy(_atomic_lock);
#endif
	return -1;
}