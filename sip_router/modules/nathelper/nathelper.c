/*$Id: nathelper.c,v 1.3 2003/04/15 20:35:48 jiri Exp $
 *
 * Ser module, it implements the following commands:
 * add_rport() - insert a rport parameter into the first Via field
 * fix_nated_contact() - replaces host:port in Contact field with host:port
 *			 we received this message from
 * fix_nated_sdp() - replaces IP address in the SDP with IP address
 *		     and/or adds direction=active option to the SDP
 *
 * Beware, those functions will only work correctly if the UA supports
 * symmetric signalling and media (not all do)!!!
 * 
 *
 * Copyright (C) 2003 Porta Software Ltd
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

#include "nhelpr_funcs.h"
#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../data_lump.h"
#include "../../data_lump_rpl.h"
#include "../../error.h"
#include "../../mem/mem.h"
#include "../../parser/parser_f.h"
#include "../../ut.h"
#include "../registrar/sip_msg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* FIXME */
#warning CAUTION: nathelper uses unknown strnstr; TEMPORARILY REMOVED
#define strnstr(_a,_b,_c) 0


static int fix_nated_contact_f(struct sip_msg*, char*, char*);
static int fix_nated_sdp_f(struct sip_msg*, char*, char*);

inline static int fixup_str2int(void**, int);

static int mod_init(void);

static cmd_export_t cmds[]={
		{"fix_nated_contact", fix_nated_contact_f, 0, 0, REQUEST_ROUTE | ONREPLY_ROUTE },
		{"fix_nated_sdp", fix_nated_sdp_f, 1, fixup_str2int, REQUEST_ROUTE | ONREPLY_ROUTE },
		{0,0,0,0,0}
	};

static param_export_t params[]={
	{0,0,0}
};

struct module_exports exports={
		"nathelper",
		cmds,
		params,
		mod_init,
		0, /* reply processing */
		0, /* destroy function */
		0, /* on_break */
		0  /* child_init */
};

static int
mod_init(void)
{
	return 0;
}

/*
 * 
 */
static int
fix_nated_contact_f(struct sip_msg* msg, char* str1, char* str2)
{
	int st, offset, len, len1;
	str hostname, port;
	char *cp, *buf, temp[2];
	contact_t* c;
	enum {ST1, ST2, ST3, ST4, ST5};
	struct lump* anchor;

	if ((parse_headers(msg, HDR_CONTACT, 0) == -1) || !msg->contact)
		return -1;
	if (!msg->contact->parsed && parse_contact(msg->contact) < 0) {
		LOG(L_ERR, "fix_nated_contact: Error while parsing Contact body\n");
		return -1;
	}
	c = ((contact_body_t*)msg->contact->parsed)->contacts;
	if (!c) {
		LOG(L_ERR, "fix_nated_contact: Error while parsing Contact body\n");
		return -1;
	}
	st = ST1;
	port.len = 0;
	for (cp = c->uri.s + 1; cp < c->uri.s + c->uri.len; cp++) {
		switch (*cp) {
		case ':':
			switch (st) {
			case ST1:
				st = ST2;
				break;
			case ST3:
				hostname.len = cp - hostname.s + 1;
				port.s = cp + 1;
				st = ST4;
				break;
			}
			break;

		case '@':
			if (st == ST2) {
				hostname.s = cp + 1;
				st = ST3;
			}
			break;

		case ';':
		case '>':
			switch (st) {
			case ST3:
				hostname.len = cp - hostname.s;
				st = ST5;
				break;
			case ST4:
				port.len = cp - port.s;
				st = ST5;
				break;
			}
			break;

		default:
			break;
		}
	}
	if (st != ST5 || hostname.len == 0) {
		LOG(L_ERR, "fix_nated_contact: Error while parsing Contact URI\n");
		return -1;
	}
	if (port.len == 0)
		port.s = hostname.s + hostname.len;

	offset = c->uri.s - msg->buf;
	anchor = del_lump(&msg->add_rm, offset, c->uri.len, HDR_CONTACT);
	if (anchor == 0)
		return -1;

	cp = ip_addr2a(&msg->rcv.src_ip);
	len = c->uri.len + strlen(cp) + 6 /* :port */ - (hostname.len + port.len) + 1;
	buf = pkg_malloc(len);
	if (buf == NULL) {
		LOG(L_ERR, "ERROR: fix_nated_contact: out of memory\n");
		return -1;
	}
	temp[0] = hostname.s[0];
	temp[1] = c->uri.s[c->uri.len];
	c->uri.s[c->uri.len] = hostname.s[0] = '\0';
	len1 = snprintf(buf, len, "%s%s:%d%s", c->uri.s, cp, msg->rcv.src_port,
	    port.s + port.len);
	if (len1 < len)
		len = len1;
	hostname.s[0] = temp[0];
	c->uri.s[c->uri.len] = temp[1];
	if (insert_new_lump_after(anchor, buf, len, HDR_CONTACT) == 0) {
		pkg_free(buf);
		return -1;
	}
	c->uri.s = buf;
	c->uri.len = len;

	return 1;
}

inline static int
fixup_str2int( void** param, int param_no)
{
	unsigned int go_to;
	int err;

	if (param_no == 1) {
		go_to = str2s(*param, strlen(*param), &err);
		if (err == 0) {
			pkg_free(*param);
			*param = (void *)go_to;
			return 0;
		} else {
			LOG(L_ERR, "ERROR: fixup_str2int: bad number <%s>\n",
				(char *)(*param));
			return E_CFG;
		}
	}
	return 0;
}

#define	ADD_ADIRECTION	0x01
#define	FIX_MEDIAIP	0x02

#define ADIRECTION	"a=direction:active\r\n"
#define	ADIRECTION_LEN	21

#define CLEN_LEN	10

static int
fix_nated_sdp_f(struct sip_msg* msg, char* str1, char* str2)
{
	str body;
	int level, added_len, offset, len, nextisip;
	char *buf, *cp, *cp1;
	struct lump* anchor;
	str mediaip;

	level = (int)str1;
	added_len = 0;

	if (extract_body(msg, &body) == -1 || body.len == 0) {
		LOG(L_ERR,"ERROR: fix_nated_sdp: cannot extract body from msg!\n");
		return -1;
	}

	if (level & ADD_ADIRECTION) {
		anchor = anchor_lump(&(msg->add_rm),
		    body.s + body.len - msg->buf, 0, 0);
		if (anchor == NULL) {
			LOG(L_ERR, "ERROR: fix_nated_sdp: anchor_lump failed\n");
			return -1;
		}
		buf = pkg_malloc(ADIRECTION_LEN * sizeof(char));
		if (buf == NULL) {
			LOG(L_ERR, "ERROR: fix_nated_sdp: out of memory\n");
			return -1;
		}
		memcpy(buf, ADIRECTION, ADIRECTION_LEN);
		if (insert_new_lump_after(anchor, buf, ADIRECTION_LEN - 1, 0) == NULL) {
			LOG(L_ERR, "ERROR: fix_nated_sdp: insert_new_lump_after failed\n");
			pkg_free(buf);
			return -1;
		}
		added_len += ADIRECTION_LEN - 1;
	}

	if (level & FIX_MEDIAIP) {
		for (cp = body.s; (len = body.s + body.len - cp) > 0;) {
			cp1 = strnstr(cp, "c=", len);
			if (cp1 == NULL || cp1[-1] == '\n' || cp1[-1] == '\r')
				break;
			cp = cp1 + 2;
		}
		if (cp1 == NULL) {
			LOG(L_ERR, "ERROR: fix_nated_sdp: no `c=' in SDP\n");
			goto finalise;
		}
		mediaip.s = cp1 + 2;
		mediaip.len = eat_line(mediaip.s, body.s + body.len - mediaip.s) - mediaip.s;
		trim_len(mediaip.len, mediaip.s, mediaip);

		nextisip = 0;
		for (cp = mediaip.s; cp < mediaip.s + mediaip.len;) {
			len = eat_token_end(cp, mediaip.s + mediaip.len) - cp;
			if (nextisip == 1) {
				mediaip.s = cp;
				mediaip.len = len;
				nextisip++;
				break;
			}
			if (len == 3 && memcmp(cp, "IP4", 3) == 0)
				nextisip = 1;
			cp = eat_space_end(cp + len, mediaip.s + mediaip.len);
		}
		if (nextisip != 2 || mediaip.len == 0) {
			LOG(L_ERR, "ERROR: fix_nated_sdp: no `IP4' in `c=' field\n");
			goto finalise;
		}

		cp = ip_addr2a(&msg->rcv.src_ip);
		len = strlen(cp);
		/* check that updating mediaip is really necessary */
		if (len == mediaip.len && memcmp(cp, mediaip.s, len) == 0)
			goto finalise;
		buf = pkg_malloc(len);
		if (buf == NULL) {
			LOG(L_ERR, "ERROR: fix_nated_sdp: out of memory\n");
			return -1;
		}
		offset = mediaip.s - msg->buf;
		anchor = del_lump(&msg->add_rm, offset, mediaip.len, 0);
		if (anchor == NULL) {
			LOG(L_ERR, "ERROR: fix_nated_sdp: del_lump failed\n");
			pkg_free(buf);
			return -1;
		}
		memcpy(buf, cp, len);
		if (insert_new_lump_after(anchor, buf, len, 0) == 0) {
			LOG(L_ERR, "ERROR: fix_nated_sdp: insert_new_lump_after failed\n");
			pkg_free(buf);
			return -1;
		}
		added_len += len - mediaip.len;
	}

finalise:
	/* Check that Content-Length needs to be updated */
	if (added_len != 0) {
		buf = pkg_malloc(CLEN_LEN * sizeof(char));
		if (buf == NULL) {
			LOG(L_ERR, "ERROR: fix_nated_sdp: out of memory\n");
			return -1;
		}
		offset = msg->content_length->body.s - msg->buf;
		len = msg->content_length->body.len;
		anchor = del_lump(&msg->add_rm, offset, len, HDR_CONTENTLENGTH);
		if (anchor == NULL) {
			LOG(L_ERR, "ERROR: fix_nated_sdp: del_lump failed\n");
			pkg_free(buf);
			return -1;
		}
		len = snprintf(buf, CLEN_LEN, "%d", body.len + added_len);
		if (len >= CLEN_LEN)
			len = CLEN_LEN - 1;
		if (insert_new_lump_after(anchor, buf, len, HDR_CONTENTLENGTH) == NULL) {
			LOG(L_ERR, "ERROR: fix_nated_sdp: insert_new_lump_after failed\n");
			pkg_free(buf);
			return -1;
		}
	}

	return 1;
}
