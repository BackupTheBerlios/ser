/*
 * $Id: sip_msg.h,v 1.12 2002/08/15 08:13:29 jku Exp $
 */


#ifndef _SIP_MSG_H
#define _SIP_MSG_H

#include "../../parser/msg_parser.h"
#include "../../mem/shm_mem.h"

#define  sip_msg_free(_p_msg) shm_free( (_p_msg ))
#define  sip_msg_free_unsafe(_p_msg) shm_free_unsafe( (_p_msg) )


struct sip_msg*  sip_msg_cloner( struct sip_msg *org_msg );


#endif
