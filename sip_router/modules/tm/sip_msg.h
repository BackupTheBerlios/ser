/*
 * $Id: sip_msg.h,v 1.9 2002/01/29 17:28:40 bogdan Exp $
 */


#ifndef _SIP_MSG_H
#define _SIP_MSG_H

#include "../../msg_parser.h"

#include "sh_malloc.h"

#define sip_msg_cloner(p_msg) \
    sip_msg_cloner_2(p_msg)

#define  sip_msg_free(_p_msg) shm_free( (_p_msg ))
#define  sip_msg_free_unsafe(_p_msg) shm_free_unsafe( (_p_msg) )


struct sip_msg*  sip_msg_cloner_1( struct sip_msg *org_msg );
struct sip_msg*  sip_msg_cloner_2( struct sip_msg *org_msg );
void                     sip_msg_free_1( struct sip_msg *org_msg );



//char*   translate_pointer( char* new_buf , char *org_buf , char* p);


#endif
