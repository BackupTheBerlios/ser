/*
 * $Id: flags.h,v 1.2 2002/05/26 13:50:48 andrei Exp $
 */


#ifndef _FLAGS_H
#define _FLAGS_H

enum { FL_WHITE=1, FL_YELLOW, FL_GREEN, FL_RED, FL_BLUE, FL_MAGENTA,
	   FL_BROWN, FL_BLACK, FL_ACC, FL_MAX };

typedef unsigned int flag_t;

#define MAX_FLAG  ( sizeof(flag_t) * CHAR_BIT - 1 )

struct sip_msg;

int setflag( struct sip_msg* msg, flag_t flag );
int resetflag( struct sip_msg* msg, flag_t flag );
int isflagset( struct sip_msg* msg, flag_t flag );

int flag_in_range( flag_t flag );

#endif
