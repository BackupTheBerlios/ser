/* 
 * PATH MODULE
 *
 * Header file for path module
 * 
 * Copyright (C) 2005, Agora Systems S. A.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 *
 * An online copy of the licence can be found at
 * http://www.gnu.org/copyleft/gpl.html
 * 
 * Authors:
 *
 * - Carlos Garcia Santos (carlos.garcia@agora-2000.com)
 * - Fermín Galán Márquez (fermin.galan@agora-2000.com)
 *
 * Agora Systems S. A., 2005 (C)
 *
 */

#ifndef _PATH_H
#define _PATH_H

/* Function prototypes */
static int mod_init(void);
static void destroy(void);

static int path_based_route(struct sip_msg* msg);
static int store_path(struct sip_msg* msg);
static int test_header(struct sip_msg* msg);
static int test_routing(struct sip_msg* msg);

#endif
