/* 
 * $Id: usrloc.h,v 1.5 2002/04/23 09:31:25 andrei Exp $ 
 */

#ifndef USRLOC_H
#define USRLOC_H

/*
 * User location module
 */
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "contact_parser.h"




extern char* db_table;    /* Database table name parameter variable */
extern char* user_col;    /* User column name parameter variable */
extern char* contact_col; /* Contact column name parameter variable */
extern char* expires_col; /* Expires column name parameter variable */
extern char* q_col;       /* q column name parameter variable */
extern char* callid_col;  /* CallID column name parameter variable */
extern char* cseq_col;    /* CSeq column name parameter variable */

#endif
