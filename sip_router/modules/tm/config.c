/*
 * $Id: config.c,v 1.1 2008/02/15 12:11:46 tirpi Exp $
 *
 * Copyright (C) 2008 iptelorg GmbH
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
 *
 * History
 * -------
 *  2008-02-05	adapting tm module for the configuration framework (Miklos)
 */

#include "../../cfg/cfg.h"
#include "../../parser/msg_parser.h" /* method types */
#include "timer.h"
#include "t_fwd.h"
#include "config.h"

struct cfg_group_tm	default_tm_cfg = {
	/* should be request-uri matching used as a part of pre-3261
	 * transaction matching, as the standard wants us to do so
	 * (and is reasonable to do so, to be able to distinguish
	 * spirals)? turn only off for better interaction with
	 * devices that are broken and send different r-uri in
	 * CANCEL/ACK than in original INVITE
	 */
	1,	/* ruri_matching */
	1,	/* via1_matching */
	FR_TIME_OUT,	/* fr_timeout */
	INV_FR_TIME_OUT,	/* fr_inv_timeout */
	WT_TIME_OUT,	/* wait_timeout */
	DEL_TIME_OUT,	/* delete_timeout */
	RETR_T1,	/* rt_t1_timeout */
	RETR_T2,	/* rt_t2_timeout */

	/* maximum time an invite or noninv transaction will live, from
	 * the moment of creation (overrides larger fr/fr_inv timeouts,
	 * extensions due to dns failover, fr_inv restart a.s.o)
	 * Note: after this time the transaction will not be deleted
	 *  immediately, but forced to go in the wait state or in wait for ack state
	 *  and then wait state, so it will still be alive for either wait_timeout in
	 *  the non-inv or "silent" inv. case and for fr_timeout + wait_timeout for an
	 *  invite transaction (for which  we must wait for the neg. reply ack)
	 */
	MAX_INV_LIFETIME,	/* tm_max_inv_lifetime */
	MAX_NONINV_LIFETIME,	/* tm_max_noninv_lifetime */
	1,	/* noisy_ctimer */
	1,	/* tm_auto_inv_100 */
	500,	/* tm_unix_tx_timeout -- 500 ms by default */
	1,	/* restart_fr_on_each_reply */
	0,	/* pass_provisional_replies */
	1,	/* tm_aggregate_auth */
	UM_CANCEL_STATEFULL,	/* unmatched_cancel */
	500,	/* default_code */
	"Server Internal Error",	/* default_reason */
	1,	/* reparse_invite */
	STR_NULL,	/* ac_extra_hdrs */

	0,	/* tm_blst_503 -- if 1 blacklist 503 sources, using tm_blst_503_min,
		 * tm_blst_503_max, tm_blst_503_default and the Retry-After header
		 * in the 503 reply */
	0,	/* tm_blst_503_default -- rfc conformant: do not blacklist if
		 * no retry-after header is present */
	0,	/* tm_blst_503 -- minimum 503 blacklist time is 0 sec */
	3600,	/* tm_blst_503_max -- maximum 503 blacklist time is 3600 sec */
	METHOD_INVITE,	/* tm_blst_methods_add -- backlist only INVITE
			 * timeouts by default */
	~METHOD_BYE,	/* tm_blst_methods_lookup -- look-up the blacklist
			 * for every method except BYE by default */
};

void	*tm_cfg = &default_tm_cfg;

cfg_def_t	tm_cfg_def[] = {
	{"ruri_matching",	CFG_VAR_INT,	0, 1, 0, 0,
		"perform Request URI check in tranaction matching"},
	{"via1_matching",	CFG_VAR_INT,	0, 1, 0, 0,
		"perform first Via header check in tranaction matching"},
	{"fr_timer",		CFG_VAR_INT,	0, 0, timer_fixup, 0,
		"timer which hits if no final reply for a request "
		"or ACK for a negative INVITE reply arrives "
		"(in milliseconds)"},
	{"fr_inv_timer",	CFG_VAR_INT,	0, 0, timer_fixup, 0,
		"timer which hits if no final reply for an INVITE arrives "
		"after a provisional message was received (in milliseconds)"},
	{"wt_timer",		CFG_VAR_INT,	0, 0, timer_fixup, 0,
		"time for which a transaction stays in memory to absorb "
		"delayed messages after it completed"},
	{"delete_timer",	CFG_VAR_INT,	0, 0, timer_fixup, 0,
		"time after which a to-be-deleted transaction currently "
		"ref-ed by a process will be tried to be deleted again."},
	{"retr_timer1",		CFG_VAR_INT,	0, 0, timer_fixup, 0,
		"initial retransmission period (in milliseconds)"},
	{"retr_timer2",		CFG_VAR_INT,	0, 0, timer_fixup, 0,
		"maximum retransmission period (in milliseconds)"},
	{"max_inv_lifetime",	CFG_VAR_INT,	0, 0, timer_fixup, 0,
		"maximum time an invite transaction can live "
		"from the moment of creation"},
	{"max_noninv_lifetime",	CFG_VAR_INT,	0, 0, timer_fixup, 0,
		"maximum time a non-invite transaction can live "
		"from the moment of creation"},
	{"noisy_ctimer",	CFG_VAR_INT,	0, 1, 0, 0,
		"if set, INVITE transactions that time-out (FR INV timer) "
		"will be always replied"},
	{"auto_inv_100",	CFG_VAR_INT,	0, 1, 0, 0,
		"automatically send 100 to an INVITE"},
	{"unix_tx_timeout",	CFG_VAR_INT,	0, 0, 0, 0,
		"Unix socket transmission timeout, in milliseconds"},
	{"restart_fr_on_each_reply",	CFG_VAR_INT,	0, 1, 0, 0,
		"restart final response timer on each provisional reply"},
	{"pass_provisional_replies",	CFG_VAR_INT,	0, 1, 0, 0,
		"enable/disable passing of provisional replies "
		"to FIFO applications"},
	{"aggregate_challenges",	CFG_VAR_INT,	0, 1, 0, 0,
		"if the final response is a 401 or a 407, aggregate all the "
		"authorization headers (challenges) "
		"(rfc3261 requires this to be on)"},
	{"unmatched_cancel",	CFG_VAR_INT,	0, 2, 0, 0,
		"determines how CANCELs with no matching transaction are handled "
		"(0: statefull forwarding, 1: stateless forwarding, 2: drop)"},
	{"default_code",	CFG_VAR_INT,	400, 699, 0, 0,
		"default SIP response code sent by t_reply(), if the function "
		"cannot retrieve its parameters"},
	{"default_reason",	CFG_VAR_STRING,	0, 0, 0, 0,
		"default SIP reason phrase sent by t_reply(), if the function "
		"cannot retrieve its parameters"},
	{"reparse_invite",	CFG_VAR_INT,	0, 1, 0, 0,
		"if set to 1, the CANCEL and negative ACK requests are "
		"constructed from the INVITE message which was sent out "
		"instead of building them from the received request"},
	{"ac_extra_hdrs",	CFG_VAR_STR,	0, 0, 0, 0,
		"header fields prefixed by this parameter value are included "
		"in the CANCEL and negative ACK messages if they were present "
		"in the outgoing INVITE (depends on reparse_invite)"},
	{"blst_503",		CFG_VAR_INT,	0, 1, 0, 0,
		"if set to 1, blacklist 503 SIP response sources"},
	{"blst_503_def_timeout",	CFG_VAR_INT,	0, 0, 0, 0,
		"default 503 blacklist time (in s), when no Retry-After "
		"header is present"},
	{"blst_503_min_timeout",	CFG_VAR_INT,	0, 0, 0, 0,
		"minimum 503 blacklist time (in s)"},
	{"blst_503_max_timeout",	CFG_VAR_INT,	0, 0, 0, 0,
		"maximum 503 blacklist time (in s)"},
	{"blst_methods_add",	CFG_VAR_INT,	0, 0, 0, 0,
		"bitmap of method types that trigger blacklisting on "
		"transaction timeouts"},
	{"blst_methods_lookup",	CFG_VAR_INT,	0, 0, 0, 0,
		"Bitmap of method types that are looked-up in the blacklist "
		"before statefull forwarding"},
	{0, 0, 0, 0, 0, 0}
};
