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

#ifndef __SUBSCRIBER_H
#define __SUBSCRIBER_H

/* Subscriber functions for notifier_domain */

#include <presence/notifier_domain.h>

#ifdef __cplusplus
extern "C" {
#endif

/* If a notifier publishing watched state registeres after subscibe
 * call, it receives the subscription automaticaly too! */
/*subscription_t *subscribe(notifier_domain_t *domain, 
		subscription_t *params);*/
subscription_t *subscribe(notifier_domain_t *domain, 
		str_t *package,
		str_t *record_id,
		str_t *subscriber_id,
		msg_queue_t *dst,
		void *subscriber_data);

/** Destroys an existing subscription */
void unsubscribe(notifier_domain_t *domain, subscription_t *s);

void set_subscriber_data(subscription_t *s, void *data);
void *get_subscriber_data(subscription_t *s);
	
#ifdef __cplusplus
}
#endif

#endif
