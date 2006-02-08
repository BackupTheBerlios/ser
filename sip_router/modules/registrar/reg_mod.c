/*
 * $Id: reg_mod.c,v 1.40 2006/02/08 12:20:28 tma0 Exp $
 *
 * Registrar module interface
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
/*
 * History:
 * --------
 *  2003-03-11  updated to the new module exports interface (andrei)
 *  2003-03-16  flags export parameter added (janakj)
 *  2003-03-21  save_noreply added, provided by Maxim Sobolev <sobomax@portaone.com> (janakj)
 *  2006-02-07  named flag support (andrei)
 */

#include <stdio.h>
#include "../../sr_module.h"
#include "../../timer.h"
#include "../../dprint.h"
#include "../../error.h"
#include "save.h"
#include "lookup.h"
#include "reply.h"
#include "reg_mod.h"


MODULE_VERSION


static int mod_init(void);                           /* Module init function */
static int fix_nat_flag( modparam_t type, void* val);
static int domain_fixup(void** param, int param_no); /* Fixup that converts domain name */
static void mod_destroy(void);

usrloc_api_t ul;            /* Structure containing pointers to usrloc functions */

int default_expires = 3600;           /* Default expires value in seconds */
qvalue_t default_q  = Q_UNSPECIFIED;  /* Default q value multiplied by 1000 */
int append_branches = 1;              /* If set to 1, lookup will put all contacts found in msg structure */
int case_sensitive  = 0;              /* If set to 1, username in aor will be case sensitive */
int nat_flag        = 4;              /* SER flag marking contacts behind NAT */
int min_expires     = 60;             /* Minimum expires the phones are allowed to use in seconds,
			               * use 0 to switch expires checking off */
int max_expires     = 0;              /* Minimum expires the phones are allowed to use in seconds,
			               * use 0 to switch expires checking off */
int max_contacts = 0;                 /* Maximum number of contacts per AOR */
int retry_after = 0;                  /* The value of Retry-After HF in 5xx replies */

int received_to_uri = 0;  /* copy received to uri, don't add it to dst_uri */

#define RCV_NAME "received"

str rcv_param = STR_STATIC_INIT(RCV_NAME);
int rcv_avp_no=42;


/*
 * sl_send_reply function pointer
 */
int (*sl_reply)(struct sip_msg* _m, char* _s1, char* _s2);


/*
 * Exported functions
 */
static cmd_export_t cmds[] = {
	{"save_contacts",   save,         1, domain_fixup, REQUEST_ROUTE                },
	{"save",            save,         1, domain_fixup, REQUEST_ROUTE                },
	{"save_noreply",    save_noreply, 1, domain_fixup, REQUEST_ROUTE                },
	{"save_memory",     save_memory,  1, domain_fixup, REQUEST_ROUTE                },
	{"lookup_contacts", lookup,       1, domain_fixup, REQUEST_ROUTE | FAILURE_ROUTE},
	{"lookup",          lookup,       1, domain_fixup, REQUEST_ROUTE | FAILURE_ROUTE},
	{"registered",      registered,   1, domain_fixup, REQUEST_ROUTE | FAILURE_ROUTE},
	{0, 0, 0, 0, 0}
};


/*
 * Exported parameters
 */
static param_export_t params[] = {
	{"default_expires", PARAM_INT, &default_expires},
	{"default_q",       PARAM_INT, &default_q      },
	{"append_branches", PARAM_INT, &append_branches},
	{"nat_flag",        PARAM_INT, &nat_flag       },
	{"nat_flag",        PARAM_STRING|PARAM_USE_FUNC, fix_nat_flag},
	{"min_expires",     PARAM_INT, &min_expires    },
	{"max_expires",     PARAM_INT, &max_expires    },
        {"received_param",  PARAM_STR, &rcv_param      },
	{"received_avp",    PARAM_INT, &rcv_avp_no     },
	{"max_contacts",    PARAM_INT, &max_contacts   },
	{"retry_after",     PARAM_INT, &retry_after    },
	{"received_to_uri", PARAM_INT, &received_to_uri},
	{0, 0, 0}
};


/*
 * Module exports structure
 */
struct module_exports exports = {
	"registrar",
	cmds,        /* Exported functions */
	0,           /* RPC methods */
	params,      /* Exported parameters */
	mod_init,    /* module initialization function */
	0,
	mod_destroy, /* destroy function */
	0,           /* oncancel function */
	0            /* Per-child init function */
};


/*
 * Initialize parent
 */
static int mod_init(void)
{
	bind_usrloc_t bind_usrloc;

	DBG("registrar - initializing\n");

             /*
              * We will need sl_send_reply from stateless
	      * module for sending replies
	      */
        sl_reply = find_export("sl_send_reply", 2, 0);
	if (!sl_reply) {
		LOG(L_ERR, "registrar: This module requires sl module\n");
		return -1;
	}

	bind_usrloc = (bind_usrloc_t)find_export("ul_bind_usrloc", 1, 0);
	if (!bind_usrloc) {
		LOG(L_ERR, "registrar: Can't bind usrloc\n");
		return -1;
	}

	     /* Normalize default_q parameter */
	if (default_q != Q_UNSPECIFIED) {
		if (default_q > MAX_Q) {
			DBG("registrar: default_q = %d, lowering to MAX_Q: %d\n", default_q, MAX_Q);
			default_q = MAX_Q;
		} else if (default_q < MIN_Q) {
			DBG("registrar: default_q = %d, raising to MIN_Q: %d\n", default_q, MIN_Q);
			default_q = MIN_Q;
		}
	}


	if (bind_usrloc(&ul) < 0) {
		return -1;
	}

	return 0;
}



/* fixes nat_flag param (resolves possible named flags) */
static int fix_nat_flag( modparam_t type, void* val)
{
	return fix_flag(type, val, "registrar", "nat_flag", &nat_flag);
}



/*
 * Convert char* parameter to udomain_t* pointer
 */
static int domain_fixup(void** param, int param_no)
{
	udomain_t* d;

	if (param_no == 1) {
		if (ul.register_udomain((char*)*param, &d) < 0) {
			LOG(L_ERR, "domain_fixup(): Error while registering domain\n");
			return E_UNSPEC;
		}

		*param = (void*)d;
	}
	return 0;
}


static void mod_destroy(void)
{
	free_contact_buf();
}
