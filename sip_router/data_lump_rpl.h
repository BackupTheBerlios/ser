/*
 * $Id: data_lump_rpl.h,v 1.3 2002/05/13 01:15:40 jku Exp $
 *
 */

#ifndef data_lump_rpl_h
#define data_lump_rpl_h

#include "parser/msg_parser.h"


struct lump_rpl
{
	str text;
	struct lump_rpl* next;
};

struct lump_rpl* build_lump_rpl( char* , int );

void add_lump_rpl(struct sip_msg * , struct lump_rpl* );

void free_lump_rpl(struct lump_rpl* );

#endif
