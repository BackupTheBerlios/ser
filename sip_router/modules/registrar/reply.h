/*
 * $Id: reply.h,v 1.8 2006/09/25 11:44:22 janakj Exp $
 *
 * Send a reply
 *
 * Copyright (C) 2001-2003 FhG Fokus
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


#ifndef REPLY_H
#define REPLY_H

#include "../../str.h"
#include "../../parser/msg_parser.h"
#include "ucontact.h"

/*
 * Send a reply
 */
int send_reply(struct sip_msg* _m);


/*
 * Build Contact HF for reply
 */
int build_contact(ucontact_t* c, str* aor);


/*
 * Release contact buffer if any
 */
void free_contact_buf(void);


/*
 * Sets the reason and contact attributes.
 */
int setup_attrs(struct sip_msg* msg);

#endif /* REPLY_H */
