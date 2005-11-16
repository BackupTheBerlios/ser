/* 
 * Copyright (C) 2005 iptelorg GmbH
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

#ifndef __NOTIFIER_H
#define __NOTIFIER_H

/* Notifier functions for notifier_domain */

#include <presence/notifier_domain.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the created notifier. Indicates error if NULL. */
notifier_t *register_notifier(
	notifier_domain_t *domain,
	const str_t *package,
	server_subscribe_func subscribe,
	server_unsubscribe_func unsubscribe,
	void *user_data);

void unregister_notifier(notifier_domain_t *domain, notifier_t *info);
	
#ifdef __cplusplus
}
#endif

#endif