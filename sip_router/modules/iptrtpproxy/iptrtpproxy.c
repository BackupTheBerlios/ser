/* $Id: iptrtpproxy.c,v 1.1 2007/07/30 21:27:58 tma0 Exp $
 *
 * Copyright (C) 2007 Tomas Mandys
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
 */

#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../data_lump.h"
#include "../../data_lump_rpl.h"
#include "../../error.h"
#include "../../forward.h"
#include "../../mem/mem.h"
#include "../../parser/parse_uri.h"
#include "../../parser/parser_f.h"
#include "../../resolve.h"
#include "../../trim.h"
#include "../../ut.h"
#include "../../msg_translator.h"
#include "../../socket_info.h"
#include "../../select.h"
#include "../../select_buf.h"
#include "../../script_cb.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/netfilter/xt_RTPPROXY.h>
#include <arpa/inet.h>

MODULE_VERSION

#define MODULE_NAME "iptrtpproxy"

/* max.number of RTP streams per session */
#define MAX_MEDIA_NUMBER 20
#define MAX_SWITCHBOARD_NAME_LEN 20

struct switchboard_item {
	str name;
	struct xt_rtpproxy_sockopt_in_switchboard in_switchboard;
	struct xt_rtpproxy_sockopt_in_alloc_session in_session;

	struct switchboard_item* next;
};

static char* global_session_ids;
static str sdp_ip;
static struct xt_rtpproxy_handle handle = {.sockfd = 0};
static struct switchboard_item* switchboards = NULL;
static int switchboard_count = 0;


static struct switchboard_item* find_switchboard(str *name) {
	struct switchboard_item* p;
	for (p = switchboards; p; p=p->next) {
		if (name->len == p->name.len && strncasecmp(p->name.s, name->s, name->len)==0) break;
	}
	return p;
}

/** if succesfull allocated sessions available @rtpproxy.session_ids
 */

static int rtpproxy_alloc_fixup(void** param, int param_no) {
	switch (param_no) {
		case 1:
			return fixup_var_int_12(param, param_no);
		case 2:
			return fixup_var_str_12(param, param_no);
		default:
			return 0;
	}
}

static int rtpproxy_update_fixup(void** param, int param_no) {
	switch (param_no) {
		case 1:
			return rtpproxy_alloc_fixup(param, param_no);
		case 2:
			return fixup_var_str_12(param, param_no);
		default:
			return 0;
	}
}

static int rtpproxy_delete_fixup(void** param, int param_no) {
	return rtpproxy_update_fixup(param, 2);
}


struct sdp_session {
	unsigned int media_count;
	struct {
		int active;
		unsigned short port;
		unsigned int ip;
		str ip_s;
		str port_s;
	} media[MAX_MEDIA_NUMBER];
};

struct ipt_session {
	struct switchboard_item *switchboard;
	unsigned int stream_count;
	struct {
		int sess_id;
		unsigned short proxy_port;
	} streams[MAX_MEDIA_NUMBER];
};

static unsigned int s2ip4(str *s) {
	struct in_addr res;
	char c2;
	c2 = s->s[s->len];
	s->s[s->len] = '\0';
	if (!inet_aton(s->s, &res)) {
		s->s[s->len] = c2;
		return 0;
	}
	s->s[s->len] = c2;
	return res.s_addr;
}

static void ip42s(unsigned int ip, str *s) {
	struct in_addr ip2 = { ip };
	s->s = inet_ntoa(ip2);
	s->len = strlen(s->s);
}

#define is_alpha(_c) (((_c) >= 'a' && (_c) <= 'z') || ((_c) >= 'A' && (_c) <= 'Z') || ((_c) >= '0' && (_c) <= '9') || ((_c) == '_') || ((_c) == '-'))

inline static int next_sdp_line(char** p, char* pend, char *ltype, str* line) {
	char *cp;
	while (*p < pend) {
		while (*p < pend && (**p == '\n' || **p == '\r')) (*p)++;
		for (cp = *p; cp < pend && *cp != '\n' && *cp != '\r'; cp++);

		if (cp-*p > 2 && (*p)[1] == '=') {
			*ltype = **p;
			line->s = (*p)+2;
			line->len = cp-line->s;
			*p = cp;
			return 0;
		}
		*p = cp;
	}
	return -1;
};

/* SDP RFC2327 */
static int parse_sdp_content(struct sip_msg* msg, struct sdp_session *sess) {
	char *p, *pend, *cp, *cp2, *lend;
	str line, cline_ip_s, body;
	int sess_fl, i, cline_count;
	char ltype, savec;
	unsigned int cline_ip;

	static str supported_media_types[] = {
		STR_STATIC_INIT("udp"),
		STR_STATIC_INIT("udptl"),
		STR_STATIC_INIT("rtp/avp"),
		STR_STATIC_INIT("rtp/savpf"),
		STR_NULL
	};
	memset(sess, 0, sizeof(*sess));
	body.s = get_body(msg);
	if (body.s==0) {
		ERR(MODULE_NAME": parse_sdp_content: failed to get the message body\n");
		return -1;
	}
	body.len = msg->len -(int)(body.s - msg->buf);
	if (body.len==0) {
		ERR(MODULE_NAME": parse_sdp_content: message body has length zero\n");
		return -1;
	}

	/* no need for parse_headers(msg, EOH), get_body will parse everything */
	if (!msg->content_type)
	{
		WARN(MODULE_NAME": parse_sdp_content: Content-TYPE header absent!"
			"let's assume the content is text/plain\n");
	}
	else {
		trim_len(line.len, line.s, msg->content_type->body);
		if (line.len != sizeof("application/sdp")-1 || strncasecmp(line.s, "application/sdp", line.len) != 0) {
			ERR(MODULE_NAME": parse_sdp_content: bad content type '%.*s'\n", line.len, line.s);
			return -1;
		}
	}

	/*
	 * Parsing of SDP body.
	 * It can contain a few session descriptions (each starts with
	 * v-line), and each session may contain a few media descriptions
	 * (each starts with m-line).
	 * We have to change ports in m-lines, and also change IP addresses in
	 * c-lines which can be placed either in session header (fallback for
	 * all medias) or media description.
	 * Ports should be allocated for any media. IPs all should be changed
	 * to the same value (RTP proxy IP), so we can change all c-lines
	 * unconditionally.
	 * There are sendonly,recvonly modifiers which signalize one-way
	 * streaming, it probably won't work but it's handled the same way,
	 * RTCP commands are still bi-directional. "Inactive" modifier
	 * is not handled anyway. See RFC3264
	 */

	p = body.s;
	pend = body.s + body.len;
	sess_fl = 0;
	sess->media_count = 0;
	cline_ip_s.s = NULL;  /* make gcc happy */
	cline_ip_s.len = 0;
	cline_ip = 0;
	cline_count = 0;
	while (p < pend) {
		if (next_sdp_line(&p, pend, &ltype, &line) < 0) break;
		switch (ltype) {
			case 'v':
				/* Protocol Version: v=0 */
				if (sess_fl != 0) {
					ERR(MODULE_NAME": parse_sdp_content: only one session allowed\n");  /* RFC3264 */
					return -1;
				}
				sess_fl = 1;
				break;
			case 'c':
				/* Connection Data: c=<network type> <address type> <connection address>, ex. c=IN IP4 224.2.17.12/127 */
				switch (sess_fl) {
					case 0:
						ERR(MODULE_NAME": parse_sdp_content: c= line is not in session section\n");
						return -1;
					case 1:
					case 2:
						cline_count++;
						if (cline_count > 1) {
							/* multicast not supported */
							if (sess_fl == 2) {
								goto invalidate;
							}
							else {
								cline_ip_s.len = 0;
							}
							break;
						}
						lend = line.s + line.len;
						cp = eat_token_end(line.s, lend);
						if (cp-line.s != 2 || memcmp(line.s, "IN", 2) != 0) {
							goto invalidate;
						}
						cp = eat_space_end(cp, lend);
						line.s = cp;
						cp = eat_token_end(cp, lend);
						if (cp-line.s != 3 || memcmp(line.s, "IP4", 3) != 0) {
							goto invalidate;
						}
						cp = eat_space_end(cp, lend);
						line.s = cp;
						cp = eat_token_end(cp, lend);
						line.len = cp-line.s;
						if (line.len == 0 || q_memchr(line.s, '/', line.len)) {
							/* multicast address not supported */
							goto invalidate;
						}
						if (sess_fl == 1) {
							cline_ip_s = line;
							cline_ip = s2ip4(&line);
						}
						else {
							sess->media[sess->media_count-1].ip = s2ip4(&line);
							sess->media[sess->media_count-1].active = 1;  /* IP may by specified by hostname */
							sess->media[sess->media_count-1].ip_s = line;
						}
						break;
					default:
						;
				}
				break;
			invalidate:
					if (sess_fl == 2) {
						sess->media[sess->media_count-1].active = 0;
					}
					break;
			case 'm':
				/* Media Announcements: m=<media> <port>[/<number of ports>] <transport> <fmt list>, eg. m=audio 49170 RTP/AVP 0 */
				switch (sess_fl) {
					case 0:
						ERR(MODULE_NAME": parse_sdp_content: m= line is not in session section\n");
						return -1;
					case 1:
					case 2:
						if (sess->media_count >= MAX_MEDIA_NUMBER) {
							ERR(MODULE_NAME": parse_sdp_content: max.number of medias (%d) exceeded\n", MAX_MEDIA_NUMBER);
							return -1;
						}
						cline_count = 0;
						sess_fl = 2;
						sess->media_count++;
						sess->media[sess->media_count-1].active = 0;
						lend = line.s + line.len;
						cp = eat_token_end(line.s, lend);
						if (cp-line.s == 0) {
							break;
						}
						cp = eat_space_end(cp, lend);
						line.s = cp;
						cp = eat_token_end(cp, lend);
						line.len = cp-line.s;
						
						cp2 = q_memchr(line.s, '/', line.len);
						if (cp2) {
							/* strip optional number of ports, if present should be 2 */
							line.len = cp2-line.s;
						}
						sess->media[sess->media_count-1].port_s = line;
						if (line.len == 0) { /* invalid port? */
							break;
						}
						savec = line.s[line.len];
						line.s[line.len] = '\0';
						sess->media[sess->media_count-1].port = atol(line.s);
						line.s[line.len] = savec;
						if (sess->media[sess->media_count-1].port == 0) {
							break;
						}
						cp = eat_space_end(cp, lend);
						
						line.s = cp;
						cp = eat_token_end(cp, lend);
						line.len = cp-line.s;
						for (i = 0; supported_media_types[i].s != NULL; i++) {
							if (line.len == supported_media_types[i].len &&
								strncasecmp(line.s, supported_media_types[i].s, line.len) == 0) {
								sess->media[sess->media_count-1].active = cline_ip_s.len != 0;  /* IP may by specified by hostname */
								sess->media[sess->media_count-1].ip_s = cline_ip_s;
								sess->media[sess->media_count-1].ip = cline_ip;
								break;
							}
						}
						break;
					default:
						;
				}

				break;
			default:
				;
		}
	}
	return 0;
}

static int prepare_lumps(struct sip_msg* msg, str* position, str* s) {
	struct lump* anchor;
	char *buf;

//ERR("'%.*s' --> '%.*s'\n", position->len, position->s, s->len, s->s);	
	anchor = del_lump(msg, position->s - msg->buf, position->len, 0);
	if (anchor == NULL) {
		ERR(MODULE_NAME": prepare_lumps: del_lump failed\n");
		return -1;
	}
	buf = pkg_malloc(s->len);
	if (buf == NULL) {
		ERR(MODULE_NAME": prepare_lumps: out of memory\n");
		return -1;
	}
	memcpy(buf, s->s, s->len);
	if (insert_new_lump_after(anchor, buf, s->len, 0) == 0) {
		ERR(MODULE_NAME": prepare_lumps: insert_new_lump_after failed\n");
		pkg_free(buf);
		return -1;
	}
	return 0;
}

static int update_sdp_content(struct sip_msg* msg, int gate_a_to_b, struct sdp_session *sdp_sess, struct ipt_session *ipt_sess) {
	int i, j;
	str s;
	/* we must apply lumps for relevant c= and m= lines */
	sdp_ip.len = 0;
	for (i=0; i<sdp_sess->media_count; i++) {
		if (sdp_sess->media[i].active) {
			for (j=0; j<i; j++) {
				if (sdp_sess->media[j].active && sdp_sess->media[i].ip_s.s == sdp_sess->media[j].ip_s.s) {
					goto cline_fixed;
				}
			}
			if (sdp_ip.len == 0) {
				/* takes 1st ip to be rewritten, for aux purposes only */
				sdp_ip = sdp_sess->media[i].ip_s;
			}
			/* apply lump for ip address in c= line */
			ip42s(ipt_sess->switchboard->in_switchboard.gate[!gate_a_to_b].ip, &s);
			if (prepare_lumps(msg, &sdp_sess->media[i].ip_s, &s) < 0)
				return -1;
	cline_fixed:
			/* apply lump for port in m= line */
			s.s = int2str(ipt_sess->streams[i].proxy_port, &s.len);
			if (prepare_lumps(msg, &sdp_sess->media[i].port_s, &s) < 0)
				return -1;
		}
	}
	return 0;
}

/* null terminated result is allocated at static buffer */
static void serialize_ipt_session(struct ipt_session* sess, str* session_ids) {
	static char buf[MAX_SWITCHBOARD_NAME_LEN+1+(5+1)*MAX_MEDIA_NUMBER+1];
	char *p;
	int i;
	buf[0] = '\0';
	p = buf;
	memcpy(p, sess->switchboard->name.s, sess->switchboard->name.len);
	p += sess->switchboard->name.len;
	*p = ':';
	p++;
	for (i=0; i<sess->stream_count; i++) {
		if (sess->streams[i].sess_id >= 0) {
			p += sprintf(p, "%u", sess->streams[i].sess_id);
		}
		*p = ',';
		p++;
	}
	p--;
	*p = '\0';
	session_ids->s = buf;
	session_ids->len = p - buf;
}

/* switchboardname [":" [sess_id] [ * ( "," [sess_id] )] ] */
static int unserialize_ipt_session(str* session_ids, struct ipt_session* sess) {
	char *p, *pend, savec;
	str s;
	memset(sess, 0, sizeof(*sess));
	p = session_ids->s;
	pend = session_ids->s+session_ids->len;
	s.s = p;
	while (p < pend && is_alpha(*p)) p++;
	s.len = p-s.s;
	sess->switchboard = find_switchboard(&s);
	if (!sess->switchboard) {
		ERR(MODULE_NAME": unserialize_ipt_session: switchboard '%.*s' not found\n", s.len, s.s);
		return -1;
	}
	if (p == pend) return 0;
	if (*p != ':') {
		ERR(MODULE_NAME": unserialize_ipt_session: colon expected near '%.*s'\n", pend-p, p);
		return -1;
	}
	do {
		if (sess->stream_count >= MAX_MEDIA_NUMBER) {
		ERR(MODULE_NAME": unserialize_ipt_session: max.media number (%d) exceeded\n", MAX_MEDIA_NUMBER);
			return -1;
		}
		p++;
		sess->stream_count++;
		sess->streams[sess->stream_count-1].sess_id = -1;
		s.s = p;
		while (p < pend && (*p >= '0' && *p <= '9')) p++;
		if (p != pend && *p != ',') {
			ERR(MODULE_NAME": unserialize_ipt_session: comma expected near '%.*s'\n", pend-p, p);
			return -1;
		}
		s.len = p-s.s;
		if (s.len > 0) {
			savec = s.s[s.len];
			s.s[s.len] = '\0';
			sess->streams[sess->stream_count-1].sess_id = atol(s.s);
			s.s[s.len] = savec;
		}
	} while (p < pend);
	return 0;
}

static void delete_ipt_sessions(struct ipt_session* ipt_sess) {
	struct xt_rtpproxy_sockopt_in_sess_id in_sess_id;
	int i, j;
	for (i=0; i < ipt_sess->stream_count; i++) {
		if (ipt_sess->streams[i].sess_id >= 0) {
			j = i;
			in_sess_id.sess_id_min = ipt_sess->streams[i].sess_id;
			in_sess_id.sess_id_max = in_sess_id.sess_id_min;
			/* group more sessions if possible */
			for (; i < ipt_sess->stream_count-1; i++) {
				if (ipt_sess->streams[i+1].sess_id >= 0) {
					if (ipt_sess->streams[i+1].sess_id == in_sess_id.sess_id_max+1) {
						in_sess_id.sess_id_max = ipt_sess->streams[i+1].sess_id;
						continue;
					}
					break;
				}
			}
			if (xt_RTPPROXY_delete_session(&handle, &ipt_sess->switchboard->in_switchboard, &in_sess_id) < 0) {
				ERR(MODULE_NAME": rtpproxy_delete: xt_RTPPROXY_delete_session error: %s (%d)\n", handle.err_str, handle.err_no);
				/* what to do ? */
			}
			/* invalidate sessions including duplicates */
			for (; j<ipt_sess->stream_count; j++) {
				if (ipt_sess->streams[j].sess_id >= in_sess_id.sess_id_min && ipt_sess->streams[j].sess_id <= in_sess_id.sess_id_max)
					ipt_sess->streams[j].sess_id = -1;
			}
		}
	}
}

inline static void fill_in_session(int gate_a_to_b, int media_idx, struct sdp_session *sdp_sess, struct ipt_session *ipt_sess, struct xt_rtpproxy_sockopt_in_alloc_session *in_session) {
	int j;
	for (j=0; j<2; j++) {
		in_session->source[gate_a_to_b].stream[j].flags =
			XT_RTPPROXY_SOCKOPT_FLAG_SESSION_ADDR |
			ipt_sess->switchboard->in_session.source[gate_a_to_b].stream[j].flags;
		in_session->source[gate_a_to_b].stream[j].learning_timeout =
			ipt_sess->switchboard->in_session.source[gate_a_to_b].stream[j].learning_timeout;
		in_session->source[gate_a_to_b].stream[j].addr.ip = sdp_sess->media[media_idx].ip;
		in_session->source[gate_a_to_b].stream[j].addr.port = sdp_sess->media[media_idx].port+j;
	}
	in_session->source[gate_a_to_b].always_learn = ipt_sess->switchboard->in_session.source[gate_a_to_b].always_learn;
}

static int rtpproxy_alloc(struct sip_msg* msg, char* _gate_a_to_b, char* _switchboard_id) {
	int gate_a_to_b;
	struct switchboard_item* si = 0;
	struct sdp_session sdp_sess;
	struct ipt_session ipt_sess;
	struct xt_rtpproxy_sockopt_in_alloc_session in_session;
	struct xt_rtpproxy_session out_session;
	str s;
	int i;

	if (get_int_fparam(&gate_a_to_b, msg, (fparam_t*) _gate_a_to_b) < 0) {
		return -1;
	}
	gate_a_to_b = gate_a_to_b == 0;  /* gate_a_to_b has index 0, gate_b_to_a 1 */
	if (get_str_fparam(&s, msg, (fparam_t*) _switchboard_id) < 0) {
		return -1;
	}
	/* switchboard must be fully qualified, it simplifies helper because it's not necessary to store full identification to session_ids - name is sufficient */
	si = find_switchboard(&s);
	if (!si) {
		ERR(MODULE_NAME": rtpproxy_alloc: switchboard '%.*s' not found\n", s.len, s.s);
		return -1;
	}
	if (parse_sdp_content(msg, &sdp_sess) < 0)
		return -1;
	memset(&ipt_sess, 0, sizeof(ipt_sess));
	ipt_sess.switchboard = si;
	memset(&in_session, 0, sizeof(in_session));
	for (i = 0; i < sdp_sess.media_count; i++) {
		ipt_sess.streams[i].sess_id = -1;
		ipt_sess.stream_count = i+1;
		if (sdp_sess.media[i].active) {
			int j;
			for (j = 0; j < i; j++) {
				/* if two media streams have equal source address than we will allocate only one ipt session */
				if (sdp_sess.media[j].active && sdp_sess.media[i].ip == sdp_sess.media[j].ip && sdp_sess.media[i].port == sdp_sess.media[j].port) {
					ipt_sess.streams[i].sess_id = ipt_sess.streams[j].sess_id;
					ipt_sess.streams[i].proxy_port = ipt_sess.streams[j].proxy_port;
					goto cont;
				}
			}
			fill_in_session(gate_a_to_b, i, &sdp_sess, &ipt_sess, &in_session);
			if (xt_RTPPROXY_alloc_session(&handle, &ipt_sess.switchboard->in_switchboard, &in_session, NULL, &out_session) < 0) {
				ERR(MODULE_NAME": rtpproxy_alloc: xt_RTPPROXY_alloc_session error: %s (%d)\n", handle.err_str, handle.err_no);
				delete_ipt_sessions(&ipt_sess);
				return -1;
			}
			ipt_sess.streams[i].sess_id = out_session.sess_id;
			ipt_sess.streams[i].proxy_port = out_session.gate[!gate_a_to_b].stream[0].port;
		cont: ;
		}
	}
	if (update_sdp_content(msg, gate_a_to_b, &sdp_sess, &ipt_sess) < 0) {
		delete_ipt_sessions(&ipt_sess);
		return -1;
	}
	serialize_ipt_session(&ipt_sess, &s);
	global_session_ids = s.s; /* it's static and null terminated */
	return 1;
}

static int rtpproxy_update(struct sip_msg* msg, char* _gate_a_to_b, char* _session_ids) {
	str session_ids;
	int gate_a_to_b, i;
	struct sdp_session sdp_sess;
	struct ipt_session ipt_sess;
	struct xt_rtpproxy_sockopt_in_sess_id in_sess_id;
	struct xt_rtpproxy_sockopt_in_alloc_session in_session;

	if (get_int_fparam(&gate_a_to_b, msg, (fparam_t*) _gate_a_to_b) < 0) {
		return -1;
	}
	gate_a_to_b = gate_a_to_b == 0;  /* gate_a_to_b has index 0, gate_b_to_a 1 */
	if (get_str_fparam(&session_ids, msg, (fparam_t*) _session_ids) < 0) {
		return -1;
	}
	if (unserialize_ipt_session(&session_ids, &ipt_sess) < 0) {
		return -1;
	}
	if (parse_sdp_content(msg, &sdp_sess) < 0)
		return -1;

	if (ipt_sess.stream_count != sdp_sess.media_count) {
		ERR(MODULE_NAME": rtpproxy_update: number of m= item in offer (%d) and answer (%d) do not correspond\n", ipt_sess.stream_count, sdp_sess.media_count);
		return -1;
	}
	/* first we check for unexpected duplicate source ports */
	for (i = 0; i < sdp_sess.media_count; i++) {
		if (ipt_sess.streams[i].sess_id >= 0 && sdp_sess.media[i].active) {
			int j;
			for (j = i+1; j < sdp_sess.media_count; j++) {
				if (ipt_sess.streams[j].sess_id >= 0 && sdp_sess.media[j].active) {
					/* if two media streams have equal source address XOR have equal session */
					if ( (sdp_sess.media[i].ip == sdp_sess.media[j].ip && sdp_sess.media[i].port == sdp_sess.media[j].port) ^
						 (ipt_sess.streams[i].sess_id == ipt_sess.streams[j].sess_id) ) {
						ERR(MODULE_NAME": rtpproxy_update: media (%d,%d) violation number\n", i, j);
						return -1;
					}
				}
			}
		}
	}

	memset(&in_session, 0, sizeof(in_session));
	for (i = 0; i < sdp_sess.media_count; i++) {
		if (ipt_sess.streams[i].sess_id >= 0) {
			in_sess_id.sess_id_min = ipt_sess.streams[i].sess_id;
			in_sess_id.sess_id_max = in_sess_id.sess_id_min;
			if (sdp_sess.media[i].active) {
				fill_in_session(gate_a_to_b, i, &sdp_sess, &ipt_sess, &in_session);
				if (xt_RTPPROXY_update_session(&handle, &ipt_sess.switchboard->in_switchboard, &in_sess_id, &in_session) < 0) {
					ERR(MODULE_NAME": rtpproxy_alloc: xt_RTPPROXY_update_session error: %s (%d)\n", handle.err_str, handle.err_no);
					/* delete all sessions ? */
					return -1;
				}
				/* we don't know proxy port - it was known when being allocated so we got from switchboard - it's not too clear solution because it requires knowledge how ports are allocated */
				ipt_sess.streams[i].proxy_port = ipt_sess.switchboard->in_switchboard.gate[!gate_a_to_b].port + 2*ipt_sess.streams[i].sess_id;
			}
			else {
				/* can we delete any session allocated during offer? */
				if (xt_RTPPROXY_delete_session(&handle, &ipt_sess.switchboard->in_switchboard, &in_sess_id) < 0) {
					ERR(MODULE_NAME": rtpproxy_update: xt_RTPPROXY_delete_session error: %s (%d)\n", handle.err_str, handle.err_no);
				}
				ipt_sess.streams[i].sess_id = -1;
			}
		}
	}
	if (update_sdp_content(msg, gate_a_to_b, &sdp_sess, &ipt_sess) < 0) {
		/* delete all sessions ? */
		return -1;
	}
	serialize_ipt_session(&ipt_sess, &session_ids);
	global_session_ids = session_ids.s; /* it's static and null terminated */
	return 1;
}

static int rtpproxy_delete(struct sip_msg* msg, char* _session_ids, char* dummy) {
	str session_ids;
	struct ipt_session ipt_sess;
	if (get_str_fparam(&session_ids, msg, (fparam_t*) _session_ids) < 0) {
		return -1;
	}
	if (unserialize_ipt_session(&session_ids, &ipt_sess) < 0) {
		return -1;
	}
	delete_ipt_sessions(&ipt_sess);
	serialize_ipt_session(&ipt_sess, &session_ids);
	global_session_ids = session_ids.s; /* it's static and null terminated */
	return 1;
}

/* @select implementation */
static int sel_rtpproxy(str* res, select_t* s, struct sip_msg* msg) {  /* dummy */
	return 0;
}

static int sel_sdp_ip(str* res, select_t* s, struct sip_msg* msg) {
	*res = sdp_ip;
	return 0;
}

static int sel_session_ids(str* res, select_t* s, struct sip_msg* msg) {
	if (!global_session_ids)
		return 1;
	res->s = global_session_ids;
	res->len = strlen(res->s);
	return 0;
}

select_row_t sel_declaration[] = {
	{ NULL, SEL_PARAM_STR, STR_STATIC_INIT(MODULE_NAME), sel_rtpproxy, SEL_PARAM_EXPECTED},
	{ sel_rtpproxy, SEL_PARAM_STR, STR_STATIC_INIT("sdp_ip"), sel_sdp_ip, 0 },
	{ sel_rtpproxy, SEL_PARAM_STR, STR_STATIC_INIT("session_ids"), sel_session_ids, 0 },

	{ NULL, SEL_PARAM_INT, STR_NULL, NULL, 0}
};

static int mod_pre_script_cb(struct sip_msg *msg, void *param) {
	sdp_ip.s = "";
	sdp_ip.len = 0;
	global_session_ids = NULL;
	return 1;
}

/* module initialization */
static int mod_init(void) {
	struct switchboard_item *si;
	int i;
	if (xt_RTPPROXY_open(&handle) < 0) goto err;
	for (si = switchboards; si; si=si->next) {
		struct xt_rtpproxy_switchboard *out_switchboard;
		if (xt_RTPPROXY_get_switchboards(&handle, &si->in_switchboard, NULL, XT_RTPPROXY_SOCKOPT_FLAG_OUT_SWITCHBOARD, &out_switchboard) < 0) {
			goto err;
		}
		/* update switchboard info, we need real ports for rtpproxy_update, it may sometimes differ from in_switchboard when addr-a=addr-b. We'll take first switchboard returned, should be always only one */
		if (!out_switchboard) {
			ERR(MODULE_NAME": switchboard '%.*s' not found in iptables\n", si->name.len, si->name.s);
			goto err2;
		}
		if (si->in_switchboard.gate[0].ip == si->in_switchboard.gate[1].ip) {
			for (i=0; i<XT_RTPPROXY_MAX_GATE; i++) {
				si->in_switchboard.gate[i].port = out_switchboard->so.gate[i].addr.port;
			}
		}
		xt_RTPPROXY_release_switchboards(&handle, out_switchboard);
	}

	register_script_cb(mod_pre_script_cb, REQ_TYPE_CB | RPL_TYPE_CB| PRE_SCRIPT_CB, 0);
	register_select_table(sel_declaration);
	return 0;
err:
	ERR(MODULE_NAME": %s (%d)\n", handle.err_str, handle.err_no);
err2:
	if (handle.sockfd >= 0) {
		xt_RTPPROXY_close(&handle);
	}
	return -1;
}

static void mod_cleanup(void) {
	if (handle.sockfd >= 0) {
		xt_RTPPROXY_close(&handle);
	}
}

static int child_init(int rank) {

	return 0;
}

#define eat_spaces(_p) \
	while( *(_p)==' ' || *(_p)=='\t' ){\
	(_p)++;}

#define DEF_PARAMS(_id,_s,_fld) \
	if ( (param_ids & (_id)) && !(param_ids & ((_id) << par_GateB)) )  \
		si->_s[1]._fld = si->_s[0]._fld; \
	if ( !(param_ids & (_id)) && (param_ids & ((_id) << par_GateB)) )  \
		si->_s[0]._fld = si->_s[1]._fld;


static int declare_switchboard(modparam_t type, void* val) {
	char *s, *c;
	int i;
	struct switchboard_item *si = NULL;
	enum param_id {
		par_GateB =		8,
		par_Name =		0x000001,
		par_Addr =		0x000100,
		par_Port =		0x000200,
		par_AlwaysLearn =	0x000400,
		par_LearningTimeout =	0x000800
	};
	#define IS_GATE_B(id) ((id &	0xFF0000)!=0)
	static struct {
		char *name;
		unsigned int id;
	} params[] = {
		{.name = "name", .id = par_Name},
		{.name = "addr-a", .id = par_Addr},
		{.name = "addr-b", .id = par_Addr << par_GateB},
		{.name = "port-a", .id = par_Port},
		{.name = "port-b", .id = par_Port << par_GateB},
		{.name = "always-learn-a", .id = par_AlwaysLearn},
		{.name = "always-learn-b", .id = par_AlwaysLearn << par_GateB},
		{.name = "learning-timeout-a", .id = par_LearningTimeout},
		{.name = "learning-timeout-b", .id = par_LearningTimeout << par_GateB},

		{.name = 0, .id = 0}
	};
	unsigned int param_ids = 0;

	if (!val) return 0;
	s = val;

	eat_spaces(s);
	if (!*s) return 0;
	/* parse param: name=;addr-a=;addr-b=;port-a=;port-b=; */
	si = pkg_malloc(sizeof(*si));
	if (!si) goto err_E_OUT_OF_MEM;
	memset(si, 0, sizeof(*si));
	while (*s) {
		str p, val;
		unsigned int id;

		c = s;
		while ( is_alpha(*c) ) {
			c++;
		}
		if (c == s) {
			ERR(MODULE_NAME": declare_switchboard: param name expected near '%s'\n", s);
			goto err_E_CFG;
		}
		p.s = s;
		p.len = c-s;
		eat_spaces(c);
		s = c;
		if (*c != '=') {
			ERR(MODULE_NAME": declare_switchboard: equal char expected near '%s'\n", s);
			goto err_E_CFG;
		}
		c++;
		eat_spaces(c);
		s = c;
		while (*c && *c != ';') c++;
		val.s = s;
		val.len = c-s;
		while (val.len > 0 && val.s[val.len-1]<=' ') val.len--;
		if (*c) c++;
		eat_spaces(c);

		id = 0;
		for (i=0; params[i].id; i++) {
			if (strlen(params[i].name)==p.len && strncasecmp(params[i].name, p.s, p.len) == 0) {
				id = params[i].id;
				break;
			}
		}
		if (!id) {
			ERR(MODULE_NAME": declare_switchboard: unknown param name '%.*s'\n", p.len, p.s);
			goto err_E_CFG;
		}
		if (param_ids & id) {
			ERR(MODULE_NAME": declare_switchboard: param '%.*s' used more than once\n", p.len, p.s);
			goto err_E_CFG;
		}

		switch (id) {
			case par_Name:
				if (val.len > MAX_SWITCHBOARD_NAME_LEN) {
					ERR(MODULE_NAME": declare_switchboard: name is too long (%d>%d)\n", val.len, MAX_SWITCHBOARD_NAME_LEN);
					goto err_E_CFG;
				}
				si->name = val;
				break;
			case par_Addr:
			case par_Addr << par_GateB:
				si->in_switchboard.gate[IS_GATE_B(id)].ip = s2ip4(&val);
				if (si->in_switchboard.gate[IS_GATE_B(id)].ip == 0) {
					goto err_E_CFG2;
				}
				break;
			case par_Port:
			case par_Port << par_GateB: {
				unsigned int u;
				if (str2int(&val, &u) < 0) {
					goto err_E_CFG2;
				}
				si->in_switchboard.gate[IS_GATE_B(id)].port = u;
				break;
			}
			case par_AlwaysLearn:
			case par_AlwaysLearn <<par_GateB: {
				unsigned int u;
				if (str2int(&val, &u) < 0) {
					goto err_E_CFG2;
				}
				si->in_session.source[IS_GATE_B(id)].always_learn = u != 0;
				break;
			}
			case par_LearningTimeout:
			case par_LearningTimeout << par_GateB:{
				unsigned int u;
				if (str2int(&val, &u) < 0) {
					goto err_E_CFG2;
				}
				if (u) {
					for (i=0; i<2; i++) {
						si->in_session.source[IS_GATE_B(id)].stream[i].learning_timeout = u;
						si->in_session.source[IS_GATE_B(id)].stream[i].flags = XT_RTPPROXY_SOCKOPT_FLAG_SESSION_LEARNING_TIMEOUT;
					}
				}
				break;
			}
			default:
				BUG(MODULE_NAME": declare_switchboard: unknown id '%x\n", id);
				goto err_E_CFG;
		}
		s = c;
		param_ids |= id;
	}

	if (find_switchboard(&si->name)) {
		ERR(MODULE_NAME": declare_switchboard: name '%.*s' already declared\n", si->name.len, si->name.s);
		goto err_E_CFG;
	}
	DEF_PARAMS(par_Addr,in_switchboard.gate,ip);
	DEF_PARAMS(par_Port,in_switchboard.gate,port);

	DEF_PARAMS(par_AlwaysLearn,in_session.source,always_learn);
	for (i=0; i<2; i++) {
		DEF_PARAMS(par_LearningTimeout,in_session.source,stream[i].learning_timeout);
	}
	si->next = switchboards;
	switchboards = si;
	switchboard_count++;

	return 0;

err_E_OUT_OF_MEM:
	ERR(MODULE_NAME": declare_switchboard(#%d): not enough pkg memory\n", switchboard_count);
	return E_OUT_OF_MEM;

err_E_CFG2:
	ERR(MODULE_NAME": declare_switchboard(#%d): parse error near \"%s\"\n", switchboard_count, s);
err_E_CFG:
	if (si) pkg_free(si);

	return E_CFG;
}

static cmd_export_t cmds[] = {
	{MODULE_NAME "_alloc",     rtpproxy_alloc,         2, rtpproxy_alloc_fixup,       REQUEST_ROUTE | ONREPLY_ROUTE },
	{MODULE_NAME "_update",    rtpproxy_update,        2, rtpproxy_update_fixup,      REQUEST_ROUTE | ONREPLY_ROUTE | FAILURE_ROUTE },
	{MODULE_NAME "_delete",    rtpproxy_delete,        1, rtpproxy_delete_fixup,      REQUEST_ROUTE | ONREPLY_ROUTE | FAILURE_ROUTE },

	{0, 0, 0, 0, 0}
};

static param_export_t params[] = {
	{"switchboard",           PARAM_STRING | PARAM_USE_FUNC, &declare_switchboard},
	{0, 0, 0}
};

struct module_exports exports = {
	MODULE_NAME,
	cmds,
	0,       /* RPC methods */
	params,
	mod_init,
	0, /* reply processing */
	mod_cleanup, /* destroy function */
	0, /* on_break */
	child_init
};

