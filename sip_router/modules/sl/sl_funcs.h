/*
 * $Id: sl_funcs.h,v 1.3 2002/05/13 01:15:41 jku Exp $
 */

#ifndef _SL_FUNCS_H
#define SL_FUNCS_H

#include "../../parser/msg_parser.h"

#define SL_RPL_WAIT_TIME  2  // in sec

int sl_startup();
int sl_send_reply(struct sip_msg*,int,char*);
int sl_filter_ACK(struct sip_msg* );


#endif


