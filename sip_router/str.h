/*
 * $Id: str.h,v 1.1 2001/11/19 12:44:51 andrei Exp $
 */

#ifndef str_h
#define str_h


struct _str{
	char* s; /* null terminated string*/
	int len; /*string len*/
};

typedef struct _str str;


#endif
