/*
 * $Id: dprint.c,v 1.4 2002/09/18 07:50:47 jku Exp $
 *
 * debug print 
 *
 */
 
#include "dprint.h"
#include "globals.h"
#include "pt.h"
 
#include <stdarg.h>
#include <stdio.h>

void dprint(char * format, ...)
{
	va_list ap;

	fprintf(stderr, "%2d(%d) ", process_no, my_pid());
	va_start(ap, format);
	vfprintf(stderr,format,ap);
	fflush(stderr);
	va_end(ap);
}
