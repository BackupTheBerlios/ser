/*
 * $Id: cfg_core.c,v 1.7 2008/04/04 08:40:53 tirpi Exp $
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
 * History
 * -------
 *  2007-12-03	Initial version (Miklos)
 *  2008-01-31  added DNS resolver parameters (Miklos)
 */

#include "dprint.h"
#ifdef USE_DST_BLACKLIST
#include "dst_blacklist.h"
#endif
#include "resolve.h"
#ifdef USE_DNS_CACHE
#include "dns_cache.h"
#endif
#if defined PKG_MALLOC || defined SHM_MEM
#include "pt.h"
#endif
#include "cfg/cfg.h"
#include "cfg_core.h"

struct cfg_group_core default_core_cfg = {
	L_DEFAULT, /*  print only msg. < L_WARN */
	LOG_DAEMON,	/* log_facility -- see syslog(3) */
#ifdef USE_DST_BLACKLIST
	/* blacklist */
	0, /* dst blacklist is disabled by default */
	DEFAULT_BLST_TIMEOUT,
	DEFAULT_BLST_MAX_MEM,
#endif
	/* resolver */
#ifdef USE_IPV6
	1,  /* dns_try_ipv6 -- on by default */
#else
	0,  /* dns_try_ipv6 -- off, if no ipv6 support */
#endif
	0,  /* dns_try_naptr -- off by default */
	3,  /* udp transport preference (for naptr) */
	2,  /* tcp transport preference (for naptr) */
	1,  /* tls transport preference (for naptr) */
	-1, /* dns_retr_time */
	-1, /* dns_retr_no */
	-1, /* dns_servers_no */
	1,  /* dns_search_list */
	1,  /* dns_search_fmatch */
	0,  /* dns_reinit */
	/* DNS cache */
#ifdef USE_DNS_CACHE
	1,  /* use_dns_cache -- on by default */
	0,  /* dns_cache_flags */
	0,  /* use_dns_failover -- off by default */
	0,  /* dns_srv_lb -- off by default */
	DEFAULT_DNS_NEG_CACHE_TTL, /* neg. cache ttl */
	DEFAULT_DNS_CACHE_MIN_TTL, /* minimum ttl */
	DEFAULT_DNS_CACHE_MAX_TTL, /* maximum ttl */
	DEFAULT_DNS_MAX_MEM, /* dns_cache_max_mem */
	0, /* dns_cache_del_nonexp -- delete only expired entries by default */
#endif
#ifdef PKG_MALLOC
	0, /* mem_dump_pkg */
#endif
#ifdef SHM_MEM
	0, /* mem_dump_shm */
#endif
};

void	*core_cfg = &default_core_cfg;

cfg_def_t core_cfg_def[] = {
	{"debug",		CFG_VAR_INT|CFG_ATOMIC,	0, 0, 0, 0,
		"debug level"},
	{"log_facility",	CFG_VAR_INT|CFG_INPUT_STRING,	0, 0, log_facility_fixup, 0,
		"syslog facility, see \"man 3 syslog\""},
#ifdef USE_DST_BLACKLIST
	/* blacklist */
	{"use_dst_blacklist",	CFG_VAR_INT,	0, 1, use_dst_blacklist_fixup, 0,
		"enable/disable destination blacklisting"},
	{"dst_blacklist_expire",	CFG_VAR_INT,	0, 0, 0, 0,
		"how much time (in s) a blacklisted destination is kept in the list"},
	{"dst_blacklist_mem",	CFG_VAR_INT,	0, 0, blst_max_mem_fixup, 0,
		"maximum shared memory amount (in KB) used for keeping the blacklisted destinations"},
#endif
	/* resolver */
#ifdef USE_DNS_CACHE
	{"dns_try_ipv6",	CFG_VAR_INT,	0, 1, dns_try_ipv6_fixup, fix_dns_flags,
#else
	{"dns_try_ipv6",	CFG_VAR_INT,	0, 1, dns_try_ipv6_fixup, 0,
#endif
		"enable/disable IPv6 DNS lookups"},
#ifdef USE_DNS_CACHE
	{"dns_try_naptr",	CFG_VAR_INT,	0, 1, 0, fix_dns_flags,
#else
	{"dns_try_naptr",	CFG_VAR_INT,	0, 1, 0, 0,
#endif
		"enable/disable NAPTR DNS lookups"},
	{"dns_udp_pref",	CFG_VAR_INT,	0, 0, 0, reinit_naptr_proto_prefs,
		"udp protocol preference when doing NAPTR lookups"},
	{"dns_tcp_pref",	CFG_VAR_INT,	0, 0, 0, reinit_naptr_proto_prefs,
		"tcp protocol preference when doing NAPTR lookups"},
	{"dns_tls_pref",	CFG_VAR_INT,	0, 0, 0, reinit_naptr_proto_prefs,
		"tls protocol preference when doing NAPTR lookups"},
	{"dns_retr_time",	CFG_VAR_INT,	0, 0, 0, resolv_reinit,
		"time in s before retrying a dns request"},
	{"dns_retr_no",		CFG_VAR_INT,	0, 0, 0, resolv_reinit,
		"number of dns retransmissions before giving up"},
	{"dns_servers_no",	CFG_VAR_INT,	0, 0, 0, resolv_reinit,
		"how many dns servers from the ones defined in "
		"/etc/resolv.conf will be used"},
	{"dns_use_search_list",	CFG_VAR_INT,	0, 1, 0, resolv_reinit,
		"if set to 0, the search list in /etc/resolv.conf is ignored"},
	{"dns_search_full_match",	CFG_VAR_INT,	0, 1, 0, 0,
		"enable/disable domain name checks against the search list "
		"in DNS answers"},
	{"dns_reinit",		CFG_VAR_INT|CFG_INPUT_INT,	1, 1, dns_reinit_fixup, resolv_reinit,
		"set to 1 in order to reinitialize the DNS resolver"},
	/* DNS cache */
#ifdef USE_DNS_CACHE
	{"use_dns_cache",	CFG_VAR_INT,	0, 1, use_dns_cache_fixup, 0,
		"enable/disable the dns cache"},
	{"dns_cache_flags",	CFG_VAR_INT,	0, 4, 0, fix_dns_flags,
		"dns cache specific resolver flags "
		"(1=ipv4 only, 2=ipv6 only, 4=prefer ipv6"},
	{"use_dns_failover",	CFG_VAR_INT,	0, 1, use_dns_failover_fixup, 0,
		"enable/disable dns failover in case the destination "
		"resolves to multiple ip addresses and/or multiple SRV records "
		"(depends on use_dns_cache)"},
	{"dns_srv_lb",		CFG_VAR_INT,	0, 1, 0, fix_dns_flags,
		"enable/disable load balancing to different srv records "
		"of the same priority based on the srv records weights "
		"(depends on dns_failover)"},
	{"dns_cache_negative_ttl",	CFG_VAR_INT,	0, 0, 0, 0,
		"time to live for negative results (\"not found\") "
		"in seconds. Use 0 to disable"},
	{"dns_cache_min_ttl",	CFG_VAR_INT,	0, 0, 0, 0,
		"minimum accepted time to live for a record, in seconds"},
	{"dns_cache_max_ttl",	CFG_VAR_INT,	0, 0, 0, 0,
		"maximum accepted time to live for a record, in seconds"},
	{"dns_cache_mem",	CFG_VAR_INT,	0, 0, dns_cache_max_mem_fixup, 0,
		"maximum memory used for the dns cache in Kb"},
	{"dns_cache_del_nonexp",	CFG_VAR_INT,	0, 1, 0, 0,
		"allow deletion of non-expired records from the cache when "
		"there is no more space left for new ones"},
#endif
#ifdef PKG_MALLOC
	{"mem_dump_pkg",	CFG_VAR_INT,	0, 0, 0, mem_dump_pkg_cb,
		"dump process memory status, parameter: pid_number"},
#endif
#ifdef SHM_MEM
	{"mem_dump_shm",	CFG_VAR_INT,	0, 0, mem_dump_shm_fixup, 0,
		"dump shared memory status"},
#endif
	{0, 0, 0, 0, 0, 0}
};
