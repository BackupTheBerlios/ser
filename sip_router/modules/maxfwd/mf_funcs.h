/*
 * $Id: mf_funcs.h,v 1.7 2002/05/15 13:43:49 bogdan Exp $
 */

#ifndef _MF_FUNCS_H
#define _MF_FUNCS_H


#include "../../parser/msg_parser.h"
#include "../../dprint.h"
#include "../../config.h"
#include "../../str.h"


int decrement_maxfwd( struct sip_msg* msg, int nr_val, str *str_val );
int add_maxfwd_header( struct sip_msg* msg , unsigned int val );
int is_maxfwd_present( struct sip_msg* msg, str *mf_value );

#endif

