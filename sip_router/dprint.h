/*
 * $Id: dprint.h,v 1.1 2001/09/03 21:27:11 andrei Exp $
 */


#ifndef dprint_h
#define dprint_h



void dprint (char* format, ...);

#ifdef NO_DEBUG
	#define DPrint(fmt, args...)
#else
	#define DPrint(fmt,args...) dprint(fmt, ## args);
#endif


#endif /* ifndef dprint_h */
