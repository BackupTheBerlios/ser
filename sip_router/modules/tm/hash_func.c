/*
 * $Id: hash_func.c,v 1.5 2001/12/05 22:14:30 jku Exp $
 */



#include "hash_func.h"
#include "../../dprint.h"

int hash( str  call_id, str cseq_nr )
{
   int  hash_code = 0;
   int  i;
    if ( call_id.len>0 )
      for( i=0 ; i<call_id.len ; hash_code+=call_id.s[i++]  );
    if ( cseq_nr.len>0 )
      for( i=0 ; i<cseq_nr.len ; hash_code+=cseq_nr.s[i++] );

   return hash_code %= TABLE_ENTRIES;
}

