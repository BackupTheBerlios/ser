/* 
 * $Id: sctp_options.c,v 1.7 2009/05/29 12:24:58 andrei Exp $
 * 
 * Copyright (C) 2008 iptelorg GmbH
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/* 
 * sctp options
 */
/*
 * History:
 * --------
 *  2008-08-07  initial version (andrei)
 *  2009-05-26  runtime cfg support (andrei)
 */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/sctp.h>
#include <errno.h>

#include "sctp_options.h"
#include "dprint.h"
#include "cfg/cfg.h"
#include "socket_info.h"
#include "sctp_server.h"

struct cfg_group_sctp sctp_default_cfg;



#ifdef USE_SCTP


static int set_autoclose(void* cfg_h, str* gname, str* name, void** val);
static int set_srto_initial(void* cfg_h, str* gname, str* name, void** val);
static int set_srto_max(void* cfg_h, str* gname, str* name, void** val);
static int set_srto_min(void* cfg_h, str* gname, str* name, void** val);

/** cfg_group_sctp description (for the config framework). */
static cfg_def_t sctp_cfg_def[] = {
	/*   name        , type |input type| chg type, min, max, fixup, proc. cbk.
	      description */
	{ "socket_rcvbuf", CFG_VAR_INT| CFG_READONLY, 512, 102400, 0, 0,
		"socket receive buffer size (read-only)" },
	{ "socket_sndbuf", CFG_VAR_INT| CFG_READONLY, 512, 102400, 0, 0,
		"socket send buffer size (read-only)" },
	{ "autoclose", CFG_VAR_INT| CFG_ATOMIC, 1, 1<<30, set_autoclose, 0,
		"seconds before closing and idle connection (must be non-zero)" },
	{ "send_ttl", CFG_VAR_INT| CFG_ATOMIC, 0, 1<<30, 0, 0,
		"milliseconds before aborting a send" },
	{ "send_retries", CFG_VAR_INT| CFG_ATOMIC, 0, MAX_SCTP_SEND_RETRIES, 0, 0,
		"re-send attempts on failure" },
	{ "srto_initial", CFG_VAR_INT| CFG_ATOMIC, 0, 1<<30, set_srto_initial, 0,
		"initial value of the retr. timeout, used in RTO calculations,"
			" in msecs" },
	{ "srto_max", CFG_VAR_INT| CFG_ATOMIC, 0, 1<<30, set_srto_max, 0,
		"maximum value of the retransmission timeout (RTO), in msecs" },
	{ "srto_min", CFG_VAR_INT| CFG_ATOMIC, 0, 1<<30, set_srto_min, 0,
		"minimum value og the retransmission timeout (RTO), in msecs" },
	{0, 0, 0, 0, 0, 0, 0}
};



void* sctp_cfg; /* sctp config handle */

#endif /* USE_SCTP */

void init_sctp_options()
{
#ifdef USE_SCTP
	sctp_get_os_defaults(&sctp_default_cfg);
#if 0
	sctp_default_cfg.so_rcvbuf=0; /* do nothing, use the kernel default */
	sctp_default_cfg.so_sndbuf=0; /* do nothing, use the kernel default */
#endif
	sctp_default_cfg.autoclose=DEFAULT_SCTP_AUTOCLOSE; /* in seconds */
	sctp_default_cfg.send_ttl=DEFAULT_SCTP_SEND_TTL;   /* in milliseconds */
	sctp_default_cfg.send_retries=DEFAULT_SCTP_SEND_RETRIES;
#endif
}



#define W_OPT_NSCTP(option) \
	if (sctp_default_cfg.option){\
		WARN("sctp_options: " #option \
			" cannot be enabled (sctp support not compiled-in)\n"); \
			sctp_default_cfg.option=0; \
	}



void sctp_options_check()
{
#ifndef USE_SCTP
	W_OPT_NSCTP(autoclose);
	W_OPT_NSCTP(send_ttl);
	W_OPT_NSCTP(send_retries);
#else
	if (sctp_default_cfg.send_retries>MAX_SCTP_SEND_RETRIES) {
		WARN("sctp: sctp_send_retries too high (%d), setting it to %d\n",
				sctp_default_cfg.send_retries, MAX_SCTP_SEND_RETRIES);
		sctp_default_cfg.send_retries=MAX_SCTP_SEND_RETRIES;
	}
#endif
}



void sctp_options_get(struct cfg_group_sctp *s)
{
#ifdef USE_SCTP
	*s=*(struct cfg_group_sctp*)sctp_cfg;
#else
	memset(s, 0, sizeof(*s));
#endif /* USE_SCTP */
}



#ifdef USE_SCTP
/** register sctp config into the configuration framework.
 * @return 0 on success, -1 on error */
int sctp_register_cfg()
{
	if (cfg_declare("sctp", sctp_cfg_def, &sctp_default_cfg, cfg_sizeof(sctp),
				&sctp_cfg))
		return -1;
	if (sctp_cfg==0){
		BUG("null sctp cfg");
		return -1;
	}
	return 0;
}



#define SCTP_SET_SOCKOPT_DECLS \
	int err; \
	struct socket_info* si

#define SCTP_SET_SOCKOPT_BODY(lev, opt_name, val, err_prefix) \
	err=0; \
	for (si=sctp_listen; si; si=si->next){ \
		err+=(sctp_setsockopt(si->socket, (lev), (opt_name), (void*)(&(val)), \
							sizeof((val)), (err_prefix))<0); \
	} \
	return -(err!=0)



static int set_autoclose(void* cfg_h, str* gname, str* name, void** val)
{
#ifdef SCTP_AUTOCLOSE
	int optval;
	SCTP_SET_SOCKOPT_DECLS;
	
	optval=(int)(long)(*val);
	SCTP_SET_SOCKOPT_BODY(IPPROTO_SCTP, SCTP_AUTOCLOSE, optval,
							"cfg: setting SCTP_AUTOCLOSE");
#else
	ERR("no SCTP_AUTOCLOSE support, please upgrade your sctp library\n");
	return -1;
#endif /* SCTP_AUTOCLOSE */
}



static int set_srto_initial(void* cfg_h, str* gname, str* name, void** val)
{
#ifdef SCTP_RTOINFO
	struct sctp_rtoinfo rto;
	SCTP_SET_SOCKOPT_DECLS;
	
	if ((int)(long)(*val)==0){ /* do nothing for 0, keep the old value */
		*val=(void*)(long)cfg_get(sctp, cfg_h, srto_initial);
		return 0;
	}
	memset(&rto, 0, sizeof(rto)); /* zero everything we don't care about */
	rto.srto_assoc_id=0; /* all */
	rto.srto_initial=(int)(long)(*val);
	SCTP_SET_SOCKOPT_BODY(IPPROTO_SCTP, SCTP_RTOINFO, rto,
							"cfg: setting SCTP_RTOINFO");
#else
	ERR("no SCTP_RTOINFO support, please upgrade your sctp library\n");
	return -1;
#endif /* SCTP_RTOINFO */
}



static int set_srto_max(void* cfg_h, str* gname, str* name, void** val)
{
#ifdef SCTP_RTOINFO
	struct sctp_rtoinfo rto;
	SCTP_SET_SOCKOPT_DECLS;
	
	if ((int)(long)(*val)==0){ /* do nothing for 0, keep the old value */
		*val=(void*)(long)cfg_get(sctp, cfg_h, srto_max);
		return 0;
	}
	memset(&rto, 0, sizeof(rto)); /* zero everything we don't care about */
	rto.srto_assoc_id=0; /* all */
	rto.srto_max=(int)(long)(*val);
	SCTP_SET_SOCKOPT_BODY(IPPROTO_SCTP, SCTP_RTOINFO, rto,
							"cfg: setting SCTP_RTOINFO");
#else
	ERR("no SCTP_RTOINFO support, please upgrade your sctp library\n");
	return -1;
#endif /* SCTP_RTOINFO */
}



static int set_srto_min(void* cfg_h, str* gname, str* name, void** val)
{
#ifdef SCTP_RTOINFO
	struct sctp_rtoinfo rto;
	SCTP_SET_SOCKOPT_DECLS;
	
	if ((int)(long)(*val)==0){ /* do nothing for 0, keep the old value */
		*val=(void*)(long)cfg_get(sctp, cfg_h, srto_min);
		return 0;
	}
	memset(&rto, 0, sizeof(rto)); /* zero everything we don't care about */
	rto.srto_assoc_id=0; /* all */
	rto.srto_min=(int)(long)(*val);
	SCTP_SET_SOCKOPT_BODY(IPPROTO_SCTP, SCTP_RTOINFO, rto,
							"cfg: setting SCTP_RTOINFO");
#else
	ERR("no SCTP_RTOINFO support, please upgrade your sctp library\n");
	return -1;
#endif /* SCTP_RTOINFO */
}

#endif /* USE_SCTP */
