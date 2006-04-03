/*
 * fast architecture specific locking
 *
 * $Id: fastlock.h,v 1.31 2006/04/03 19:03:16 andrei Exp $
 *
 * 
 *
 * Copyright (C) 2001-2003 FhG Fokus
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
/*
 *
 *History:
 *--------
 *  2002-02-05  created by andrei
 *  2003-01-16  added PPC locking code contributed by Dinos Dorkofikis
 *               <kdor@intranet.gr>
 *  2004-09-12  added MIPS locking for ISA>=2 (>r3000)  (andrei)
 *  2004-12-16  for now use the same locking code for sparc32 as for sparc64
 *               (it will work only if NOSMP is defined) (andrei)
 *  2005-04-27  added alpha locking code (andrei)
 *  2005-05-25  PPC locking code enabled for PPC64; added a lwsync to
 *               the tsl part and replaced the sync with a lwsync for the
 *               unlock part (andrei)
 *  2006-03-08  mips2 NOSMP (skip sync), optimized x86 & mips clobbers and
 *               input/output constraints (andrei)
 *  2006-04-03  optimization: call lock_get memory barrier outside tsl,in the 
 *               calling function, only if the lock operation succeeded
 *               (membar_getlock()) (andrei)
 *              added try_lock(); more x86 optimizations, x86  release_lock
 *               fix (andrei)
 *
 */


#ifndef fastlock_h
#define fastlock_h

#ifdef HAVE_SCHED_YIELD
#include <sched.h>
#else
#include <unistd.h>
	/* fake sched_yield */
	#define sched_yield()	sleep(0)
#endif



#define SPIN_OPTIMIZE /* if defined optimize spining on the lock:
                         try first the lock with non-atomic/non memory locking
                         operations, and only if the lock appears to be free
                         switch to the more expensive version */

typedef  volatile int fl_lock_t;



#define init_lock( l ) (l)=0


/* what membar to use (if any) after taking a lock. This
 *  was separated from the lock code to allow better optimizations.
 *  e.g.: use the membar_getlock only after getting the lock and don't use 
 *  it if lock_get fails / when spinning on tsl.
 *  There is no corresponding membar_release_lock (because lock_release
 *  must always include the needed memory barrier).
 *  WARNING: this is intended only for internal fastlock use*/
#if defined(__CPU_i386) || defined(__CPU_x86_64)
#define membar_getlock()   /* not needed on x86 */
#elif defined(__CPU_sparc64) || defined(__CPU_sparc)
#ifndef NOSMP
#define membar_getlock() \
	asm volatile ("membar #StoreStore | #StoreLoad \n\t" : : : "memory");
#else
/* no need for a compiler barrier, that is already included in lock_get/tsl*/
#define membar_getlock() /* not needed if no smp*/
#endif /* NOSMP */
#elif defined __CPU_arm || defined __CPU_arm6
#error "FIXME: check arm6 membar"
#define membar_getlock() 
#elif defined(__CPU_ppc) || defined(__CPU_ppc64)
#ifndef NOSMP
#define membar_getlock() \
	asm volatile("lwsync \n\t" : : : "memory");
#else
#define membar_getlock() 
#endif /* NOSMP */
#elif defined __CPU_mips2 || ( defined __CPU_mips && defined MIPS_HAS_LLSC ) \
	|| defined __CPU_mips64
#ifndef NOSMP
#define membar_getlock() \
	asm volatile("sync \n\t" : : : "memory");
#else
#define membar_getlock() 
#endif /* NOSMP */
#elif defined __CPU_alpha
#ifndef NOSMP
#define membar_getlock() \
	asm volatile("mb \n\t" : : : "memory");
#else
#define membar_getlock() 
#endif /* NOSMP */
#else
#error "unknown architecture"
#endif



/*test and set lock, ret 1 if lock held by someone else, 0 otherwise
 * WARNING: no memory barriers included, if you use this function directly
 *          (not recommended) and it gets the lock (ret==0), you should call 
 *          membar_getlock() after it */
inline static int tsl(fl_lock_t* lock)
{
	int val;

#if defined(__CPU_i386) || defined(__CPU_x86_64)

#ifdef NOSMP
	asm volatile(
		" xor %0, %0 \n\t"
		" btsl $0, %2 \n\t"
		" setc %b0 \n\t"
		: "=q" (val), "=m" (*lock) : "m"(*lock) : "memory", "cc"
	);
#else
	asm volatile(
#ifdef SPIN_OPTIMIZE
		" cmpb $0, %2 \n\t"
		" mov $1, %0 \n\t"
		" jnz 1f \n\t"
#else
		" mov $1, %0 \n\t"
#endif
		" xchgb %2, %b0 \n\t"
		"1: \n\t"
		: "=q" (val), "=m" (*lock) : "m"(*lock) : "memory"
	);
#endif /*NOSMP*/
#elif defined(__CPU_sparc64) || defined(__CPU_sparc)
	asm volatile(
			"ldstub [%1], %0 \n\t"
			/* membar_getlock must be  called outside this function */
			: "=r"(val) : "r"(lock):"memory"
	);
	
#elif defined __CPU_arm || defined __CPU_arm6
	asm volatile(
			"# here \n\t"
			"swpb %0, %1, [%2] \n\t"
			: "=r" (val)
			: "r"(1), "r" (lock) : "memory"
	);
	
#elif defined(__CPU_ppc) || defined(__CPU_ppc64)
	asm volatile(
			"1: lwarx  %0, 0, %2\n\t"
			"   cmpwi  %0, 0\n\t"
			"   bne    0f\n\t"
			"   stwcx. %1, 0, %2\n\t"
			"   bne-   1b\n\t"
			/* membar_getlock must be  called outside this function */
			"0:\n\t"
			: "=r" (val)
			: "r"(1), "b" (lock) :
			"memory", "cc"
        );
#elif defined __CPU_mips2 || ( defined __CPU_mips && defined MIPS_HAS_LLSC ) \
	|| defined __CPU_mips64
	long tmp;
	
	asm volatile(
		".set push \n\t"
		".set noreorder\n\t"
		".set mips2 \n\t"
		"1:  ll %1, %2   \n\t"
		"    li %0, 1 \n\t"
		"    sc %0, %2  \n\t"
		"    beqz %0, 1b \n\t"
		"    nop \n\t"
		/* membar_getlock must be called outside this function */
		".set pop\n\t"
		: "=&r" (tmp), "=&r" (val), "=m" (*lock) 
		: "m" (*lock) 
		: "memory"
	);
#elif defined __CPU_alpha
	long tmp;
	tmp=0;
	/* lock low bit set to 1 when the lock is hold and to 0 otherwise */
	asm volatile(
		"1:  ldl %0, %1   \n\t"
		"    blbs %0, 2f  \n\t"  /* optimization if locked */
		"    ldl_l %0, %1 \n\t"
		"    blbs %0, 2f  \n\t" 
		"    lda %2, 1    \n\t"  /* or: or $31, 1, %2 ??? */
		"    stl_c %2, %1 \n\t"
		"    beq %2, 1b   \n\t"
		/* membar_getlock must be called outside this function */
		"2:               \n\t"
		:"=&r" (val), "=m"(*lock), "=r"(tmp)
		:"m"(*lock) 
		: "memory"
	);
#else
#error "unknown architecture"
#endif
	return val;
}



inline static void get_lock(fl_lock_t* lock)
{
#ifdef ADAPTIVE_WAIT
	int i=ADAPTIVE_WAIT_LOOPS;
#endif
	
	while(tsl(lock)){
#ifdef BUSY_WAIT
#elif defined ADAPTIVE_WAIT
		if (i>0) i--;
		else sched_yield();
#else
		sched_yield();
#endif
	}
	membar_getlock();
}



/* like get_lock, but it doesn't wait. If it gets the lock returns 0,
 *  <0  otherwise (-1) */
inline static int try_lock(fl_lock_t* lock)
{
	if (tsl(lock)){
		return -1;
	}
	membar_getlock();
	return 0;
}



inline static void release_lock(fl_lock_t* lock)
{
#if defined(__CPU_i386) 
#ifdef NOSMP
	asm volatile(
		" movb $0, %0 \n\t" 
		: "=m"(*lock) : : "memory"
	); 
#else /* ! NOSMP */
	int val;
	/* a simple mov $0, (lock) does not force StoreStore ordering on all
	   x86 versions and it doesn't seem to force LoadStore either */
	asm volatile(
		" xchgb %b0, %1 \n\t"
		: "=q" (val), "=m" (*lock) : "0" (0) : "memory"
	);
#endif /* NOSMP */
#elif defined(__CPU_x86_64)
	asm volatile(
		" movb $0, %0 \n\t" /* on amd64 membar StoreStore | LoadStore is 
							   implicit (at least on the same mem. type) */
		: "=m"(*lock) : : "memory"
	);
#elif defined(__CPU_sparc64) || defined(__CPU_sparc)
	asm volatile(
#ifndef NOSMP
			"membar #LoadStore | #StoreStore \n\t" /*is this really needed?*/
#endif
			"stb %%g0, [%0] \n\t"
			: /*no output*/
			: "r" (lock)
			: "memory"
	);
#elif defined __CPU_arm || defined __CPU_arm6
	asm volatile(
		" str %0, [%1] \n\r" 
		: /*no outputs*/ 
		: "r"(0), "r"(lock)
		: "memory"
	);
#elif defined(__CPU_ppc) || defined(__CPU_ppc64)
	asm volatile(
			/* "sync\n\t"  lwsync is faster and will work
			 *             here too
			 *             [IBM Prgramming Environments Manual, D.4.2.2]
			 */
			"lwsync\n\t"
			"stw %0, 0(%1)\n\t"
			: /* no output */
			: "r"(0), "b" (lock)
			: "memory"
	);
#elif defined __CPU_mips2 || ( defined __CPU_mips && defined MIPS_HAS_LLSC ) \
	|| defined __CPU_mips64
	asm volatile(
		".set push \n\t"
		".set noreorder \n\t"
		".set mips2 \n\t"
#ifndef NOSMP
		"    sync \n\t"
#endif
		"    sw $0, %0 \n\t"
		".set pop \n\t"
		: "=m" (*lock)  : /* no input */ : "memory"
	);
#elif defined __CPU_alpha
	asm volatile(
		"    mb          \n\t"
		"    stl $31, %0 \n\t"
		: "=m"(*lock) :/* no input*/ : "memory"  /* because of the mb */
	);  
#else
#error "unknown architecture"
#endif
}



#endif
