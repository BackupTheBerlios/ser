/*
 * $Id: dprint.c,v 1.2 2001/12/12 23:59:36 andrei Exp $
 *
 * debug print 
 *
 */
 
#include "dprint.h"
 
#include <stdarg.h>
#include <stdio.h>

void dprint(char * format, ...)
{
	va_list ap;

	fprintf(stderr, "%2d(%d) ", process_no, pids[process_no]);
	va_start(ap, format);
	vfprintf(stderr,format,ap);
	fflush(stderr);
	va_end(ap);
}
