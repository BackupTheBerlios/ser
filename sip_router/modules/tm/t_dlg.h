/*
 * $Id: t_dlg.h,v 1.1 2002/08/15 08:13:30 jku Exp $
 *
 */

#ifndef _T_DLG_H
#define _T_DLG_H

#include "../../parser/msg_parser.h"

struct dialog {
	int place_holder;
};

int t_newdlg( struct sip_msg *msg );
struct dialog *t_getdlg() ;

#endif
