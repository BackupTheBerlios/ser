/*
 * $Id: sl_func.h,v 1.2 2002/05/11 21:30:08 jku Exp $
 */

#ifndef _SL_FUNC_H
#define SL_FUNC_H

#include "../../parse_msg.h"


int st_startup();
int st_send_reply(struct sip_msg*,int,char*);
int st_filter_ACK();


#endif


