/*
 * $Id: defs.h,v 1.10 2002/06/10 17:02:53 janakj Exp $
 */

#ifndef DEFS_H
#define DEFS_H

#define MAX_CONTACT_BUFFER 1024

#define DEFAULT_CACHE_SIZE 512

/*
 * Needed for some broken clients, if defined,
 * registers with the same callid but lower cseq
 * will be accepted
 */
#define OOO_HACK

#endif
