/*
 * $Id: config.h,v 1.22 2003/02/28 14:12:26 jiri Exp $
 *
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


#ifndef _TM_CONFIG_H
#define _TM_CONFIG_H

#include "defs.h"

/* this is where table size is defined now -- sort of
   ugly, core should not be bothered by TM table size,
   but on the other, core's stateless forwarding should 
   have consistent branch generation with stateful mode
   and needs to calculate branch/hash, for which table size
   is needed 
*/
#include "../../hash_func.h"

/* maximumum length of localy generated acknowledgement */
#define MAX_ACK_LEN   1024

/* FINAL_RESPONSE_TIMER ... tells how long should the transaction engine
   wait if no final response comes back*/
#define FR_TIME_OUT       30
#define INV_FR_TIME_OUT   120

/* WAIT timer ... tells how long state should persist in memory after
   a transaction was finalized*/
#define WT_TIME_OUT       5

/* DELETE timer ... tells how long should the transaction persist in memory
   after it was removed from the hash table and before it will be deleted */
#define DEL_TIME_OUT      2
 
/* retransmission timers */
#define RETR_T1           1
#define RETR_T2           4

/* when first reply is sent, this additional space is allocated so that
   one does not have to reallocate share memory when the message is
   replaced by a subsequent, longer message
*/
#define REPLY_OVERBUFFER_LEN 160
#define TAG_OVERBUFFER_LEN 32

/* dimensions of FIFO server */
#define MAX_METHOD	64
#define MAX_HEADER	1024
#define MAX_BODY	1024
#define MAX_DST	512
#define MAX_FROM 512

/* messages generated by server */
#define CANCELLING "cancelling"
#define CANCEL_DONE "ok -- no more pending branches"
#define CANCELLED "Request cancelled"

/* ACKnowledgement forking hack -- that is good for phones
   which send ACKs to the same outbound proxy; if forking is
   enabled, the proxy wouldn't know to which branch to forward;
   without forking, it could forward to other branch than from
   which a reply came back, resulting in UAS never seeing it;
   this hack has not been tested yet
*/
#undef ACK_FORKING_HACK

/* to-tag separator for stateful processing */
#define TM_TAG_SEPARATOR '-'

#endif
