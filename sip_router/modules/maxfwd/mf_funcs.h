/*
 * $Id: mf_funcs.h,v 1.6 2002/05/13 01:15:40 jku Exp $
 */

#ifndef _MF_FUNCS_H
#define _MF_FUNCS_H


#include "../../parser/msg_parser.h"
#include "../../dprint.h"
#include "../../config.h"


int mf_startup();
int decrement_maxfwd( struct sip_msg* msg );
int add_maxfwd_header( struct sip_msg* msg , unsigned int val );
int is_maxfwd_zero( struct sip_msg* msg );
int reply_to_maxfwd_zero( struct sip_msg* msg );
int is_maxfwd_present( struct sip_msg* msg );

#endif

