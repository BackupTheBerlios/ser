/*
 * $Id: sl_funcs.h,v 1.4 2002/05/26 17:53:21 jku Exp $
 */

#ifndef _SL_FUNCS_H
#define SL_FUNCS_H

#include "../../parser/msg_parser.h"

#define TOTAG_SEPARATOR		'.'

#define SL_RPL_WAIT_TIME  2  // in sec

#define TOTAG_LEN MD5_LEN+CRC16_LEN+1

int sl_startup();
int sl_send_reply(struct sip_msg*,int,char*);
int sl_filter_ACK(struct sip_msg* );


#endif


