/*
 * $Id: parser_f.c,v 1.1 2002/05/13 01:15:41 jku Exp $
 *
 * parser helper  functions
 *
 */

#include  "parser_f.h"
#include "../ut.h"

/* returns pointer to next line or after the end of buffer */
char* eat_line(char* buffer, unsigned int len)
{
	char* nl;
	char c;

	/* jku .. replace for search with a library function; not conformant
 		  as I do not care about CR
	*/
	nl=(char *)q_memchr( buffer, '\n', len );
	if ( nl ) { 
		c=* nl;
		if ( nl + 1 < buffer+len)  nl++;
		if (( nl+1<buffer+len) && * nl=='\r')  nl++;
	} else  nl=buffer+len;
	return nl;
}

