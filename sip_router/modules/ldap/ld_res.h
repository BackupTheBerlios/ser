/* 
 * $Id: ld_res.h,v 1.1 2008/05/12 12:14:38 janakj Exp $ 
 *
 * LDAP Database Driver for SER
 *
 * Copyright (C) 2008 iptelorg GmbH
 *
 * This file is part of SER, a free SIP server.
 *
 * SER is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version
 *
 * For a license to use the ser software under conditions other than those
 * described here, or to purchase support for this software, please contact
 * iptel.org by e-mail at the following addresses: info@iptel.org
 *
 * SER is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LD_RES_H
#define _LD_RES_H

/** \addtogroup ldap
 * @{ 
 */

/** \file
 * Data structures and functions to convert results obtained from LDAP
 * servers.
 */

#include "../../db/db_drv.h"
#include "../../db/db_res.h"

#include <ldap.h>

struct ld_res {
	db_drv_t gen;
	LDAPMessage* msg;
	LDAPMessage* current;
};

int ld_res(db_res_t* res);

/** @} */

#endif /* _LD_RES_H */
