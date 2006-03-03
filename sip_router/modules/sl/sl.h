/*
 * $Id: sl.h,v 1.1 2006/03/01 15:59:35 janakj Exp $
 *
 * Copyright (C) 2001-2006 FhG FOKUS
 *
 * This file is part of ser, a free SIP server.
 *
 * ser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * For a license to use the ser software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact iptel.org by e-mail at the following addresses:
 *    info@iptel.org
 *
 * ser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _SL_H
#define _SL_H

#include "sl_funcs.h"

typedef struct sl_api {
	sl_send_reply_f reply;  /* Send stateless reply */
} sl_api_t;

typedef int (*bind_sl_t)(sl_api_t* api);

#endif /* _SL_H */