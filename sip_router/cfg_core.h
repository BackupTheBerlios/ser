/*
 * $Id: cfg_core.h,v 1.8 2008/12/18 16:06:04 andrei Exp $
 *
 * Copyright (C) 2007 iptelorg GmbH
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
 * HOWTO:
 *	If you need a new configuration variable within the core, put it into
 *	struct cfg_goup_core, and define it in cfg_core.c:core_cfg_def array.
 *	The default value of the variable must be inserted into
 *	cfg_core.c:default_core_cfg
 *	Include this header file in your source code, and retrieve the
 *	value with cfg_get(core, core_cfg, variable_name).
 *
 * History
 * -------
 *  2007-12-03	Initial version (Miklos)
 */

#ifndef _CFG_CORE_H
#define _CFG_CORE_H

#include "cfg/cfg.h"

extern void	*core_cfg;

struct cfg_group_core {
	int	debug;
	int	log_facility;
#ifdef USE_DST_BLACKLIST
	/* blacklist */
	int	use_dst_blacklist; /* 1 if blacklist is enabled */
	unsigned int	blst_timeout; /* blacklist entry ttl */
	unsigned int	blst_max_mem; /* maximum memory used for the
					blacklist entries */
#endif
	/* resolver */
	int dns_try_ipv6;
	int dns_try_naptr;
	int dns_udp_pref;
	int dns_tcp_pref;
	int dns_tls_pref;
	int dns_sctp_pref;
	int dns_retr_time;
	int dns_retr_no;
	int dns_servers_no;
	int dns_search_list;
	int dns_search_fmatch;
	int dns_reinit;
	/* DNS cache */
#ifdef USE_DNS_CACHE
	int use_dns_cache;
	int dns_cache_flags;
	int use_dns_failover;
	int dns_srv_lb;
	unsigned int dns_neg_cache_ttl;
	unsigned int dns_cache_min_ttl;
	unsigned int dns_cache_max_ttl;
	unsigned int dns_cache_max_mem;
	int dns_cache_del_nonexp;
#endif
#ifdef PKG_MALLOC
	int mem_dump_pkg;
#endif
#ifdef SHM_MEM
	int mem_dump_shm;
#endif
	int udp_mtu; /**< maximum send size for udp, if > try another protocol*/
	int udp_mtu_try_proto; /**< if packet> udp_mtu, try proto (e.g. TCP) */
	int force_rport; /**< if set rport will always be forced*/
};

extern struct cfg_group_core default_core_cfg;
extern cfg_def_t core_cfg_def[];

#endif /* _CFG_CORE_H */
