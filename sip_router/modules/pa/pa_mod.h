/*
 * Presence Agent, module interface
 *
 * $Id: pa_mod.h,v 1.5 2004/01/21 18:16:03 jamey Exp $
 *
 * Copyright (C) 2001-2003 Fhg Fokus
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

#ifndef PA_MOD_H
#define PA_MOD_H

#include "../../parser/msg_parser.h"
#include "../tm/tm_load.h"

extern int default_expires;
extern int timer_interval;

/* TM bind */
extern struct tm_binds tmb;

/* PA database */
extern int use_db;
extern int use_place_table;
extern str db_url;
extern str pa_domain;
extern char *presentity_table;
extern char *watcherinfo_table;
extern char *place_table;
extern int use_bsearch;
extern int use_location_package;
extern int new_watcher_pending;

#endif /* PA_MOD_H */
