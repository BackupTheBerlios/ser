/*
 * $Id: data_lump_rpl.h,v 1.2 2002/03/02 02:20:00 andrei Exp $
 *
 */

#ifndef data_lump_rpl_h
#define data_lump_rpl_h

#include "msg_parser.h"


struct lump_rpl
{
	str text;
	struct lump_rpl* next;
};

struct lump_rpl* build_lump_rpl( char* , int );

void add_lump_rpl(struct sip_msg * , struct lump_rpl* );

void free_lump_rpl(struct lump_rpl* );

#endif
