/*
 * $Id: sh_malloc.h,v 1.4 2001/12/05 22:14:30 jku Exp $
 */


#ifndef _SH_MALLOC_H
#define _SH_MALLOC_H

#include "../../shm_mem.h"

/*
#if defined SHM_MEM

#include "../../shm_mem.h"

#define sh_malloc(size)		shm_malloc((size))
#define sh_free(ptr)		shm_free((ptr))

#else
*/

#include <stdlib.h>

#define sh_malloc(size)		shm_malloc((size))
#define sh_free(ptr)		shm_free((ptr))

#endif

/*
#ifdef MEM_DBG

#include <stdlib.h>

#define sh_malloc(size)		({ void *_p=malloc((size)); \
				  printf("MEMDBG: malloc (%d): %p\n", (size), _p); \
				  _p; })

#define sh_free(ptr)		({ printf("MEMDBG: free: %p\n", (ptr)); free((ptr)); })


#endif
*/
