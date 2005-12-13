/*
 * Route & Record-Route module, avp cookie support
 *
 * $Id: avp_cookie.c,v 1.1 2005/12/12 23:18:27 tma0 Exp $
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

#include <stdio.h>
#include "avp_cookie.h"
#include "../../crc.h"
#include "../../usr_avp.h"
#include "../../mem/mem.h"

static avp_list_t dialog_avp_list = 0;

#define AVP_COOKIE_NAME "avp="
#define AVP_COOKIE_BUFFER 1024
#define CRC_LEN 4

int rr_before_script_cb(struct sip_msg *msg, void *param) {
	DBG("rr_before_script_cb: inquired\n");
	destroy_avp_list(&dialog_avp_list);
	return 1;
}

int rr_add_avp_cookie(struct sip_msg *msg, char *param1, char *param2) {
	avp_save_item_t *re;
	int_str avp_id, avp_id2, avp_val;
	struct usr_avp *avp;
	struct search_state st;

	DBG("rr_add_avp_cookie: inquired\n");
	re = (avp_save_item_t*) param1;
	switch (re->type) {
		case 0:
			avp_id.n = re->u.n;
			break;
		case AVP_NAME_RE:
			avp_id.re = &re->u.re;
			break;
		case AVP_NAME_STR:
			avp_id.s = re->u.s;
			break;
	}

	// copy AVPs to private AVP list
	for ( avp=search_first_avp(re->type, avp_id, &avp_val, &st);
			avp;
			avp = search_next_avp(&st, &avp_val) ) {

		if ((avp->flags&(AVP_NAME_STR|AVP_VAL_STR)) == AVP_NAME_STR) {
			/* avp type str, int value */
			avp_id2.s = ((struct str_int_data*)&(avp->data))->name;
		}
		else if ((avp->flags&(AVP_NAME_STR|AVP_VAL_STR)) == (AVP_NAME_STR|AVP_VAL_STR)) {
			/* avp type str, str value */
			avp_id2.s = ((struct str_str_data*)&(avp->data))->name;
		}
		else {
			avp_id2.n = avp->id;
		}

		// set avp from cookie
		DBG("rr:rr_add_avp_cookie: coping AVP\n");
		if ( add_avp_list(&dialog_avp_list, avp->flags, avp_id2, avp_val)!=0 ) {
			LOG(L_ERR, "ERROR: rr:rr_add_avp_cookie: add_avp failed\n");
		}
	}

	return 1;
}

void base64decode(char* src_buf, int src_len, char* tgt_buf, int* tgt_len) {
	int pos, i, n;
	unsigned char c[4];
	for (pos=0, i=0, *tgt_len=0; pos < src_len; pos++) {
		if (src_buf[pos] >= 'A' && src_buf[pos] <= 'Z')
			c[i] = src_buf[pos] - 65;   // <65..90>  --> <0..25>
		else if (src_buf[pos] >= 'a' && src_buf[pos] <= 'z')
			c[i] = src_buf[pos] - 71;   // <97..122>  --> <26..51>
		else if (src_buf[pos] >= '0' && src_buf[pos] <= '9')
			c[i] = src_buf[pos] + 4;   // <48..56>  --> <52..61>
		else if (src_buf[pos] == '+')
			c[i] = 62;
		else if (src_buf[pos] == '/')
			c[i] = 63;
		else  // '='
			c[i] = 64;
		i++;
		if (i==4) {
			if (c[0] == 64)
				n = 0;
			else if (c[2] == 64)
				n = 1;
			else if (c[3] == 64)
				n = 2;
			else
				n = 3;
			switch (n) {
				case 3:
					tgt_buf[*tgt_len+2] = (char) (((c[2] & 0x03) << 6) | c[3]);
					// no break
				case 2:
					tgt_buf[*tgt_len+1] = (char) (((c[1] & 0x0F) << 4) | (c[2] >> 2));
					// no break
				case 1:
					tgt_buf[*tgt_len+0] = (char) ((c[0] << 2) | (c[1] >> 4));
					break;
			}
			i=0;
			*tgt_len+= n;
		}
	}
}

void base64encode(char* src_buf, int src_len, char* tgt_buf, int* tgt_len) {
	static char code64[64+1] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int pos;
	for (pos=0, *tgt_len=0; pos < src_len; pos+=3, *tgt_len+=4) {
		tgt_buf[*tgt_len+0] = code64[(unsigned char)src_buf[pos+0] >> 2];
		tgt_buf[*tgt_len+1] = code64[(((unsigned char)src_buf[pos+0] & 0x03) << 4) | ((pos+1 < src_len)?((unsigned char)src_buf[pos+1] >> 4):0)];
		if (pos+1 < src_len)
			tgt_buf[*tgt_len+2] = code64[(((unsigned char)src_buf[pos+1] & 0x0F) << 2) | ((pos+2 < src_len)?((unsigned char)src_buf[pos+2] >> 6):0)];
		else
			tgt_buf[*tgt_len+2] = '=';
		if (pos+2 < src_len)
			tgt_buf[*tgt_len+3] = code64[(unsigned char)src_buf[pos+2] & 0x3F];
		else
			tgt_buf[*tgt_len+3] = '=';
	}
}

str *rr_get_avp_cookies(void) {
	unsigned short crc, ll;
	static char buf[AVP_COOKIE_BUFFER];
	int len, l;
	struct usr_avp *avp;
	int_str avp_val;
	str *avp_name;
	str *result = 0;

	len = sizeof(crc);
	for ( avp=dialog_avp_list; avp; avp = avp->next ) {

		if ((avp->flags&(AVP_NAME_STR|AVP_VAL_STR)) == AVP_NAME_STR) {
			/* avp type str, int value */
			avp_name = & ((struct str_int_data*)&(avp->data))->name;
		}
		else if ((avp->flags&(AVP_NAME_STR|AVP_VAL_STR)) == (AVP_NAME_STR|AVP_VAL_STR)) {
			/* avp type str, str value */
			avp_name = & ((struct str_str_data*)&(avp->data))->name;
		}
		else
			avp_name = 0;  // dummy

		l = sizeof(char);
		if (avp->flags & AVP_NAME_STR )
			l += avp_name->len+sizeof(unsigned short);
		else
			l += sizeof(avp->id);
		if (avp->flags & AVP_VAL_STR )
			l += avp_val.s.len+sizeof(unsigned short);
		else
			l += sizeof(avp_val.n);
		if (len+l > AVP_COOKIE_BUFFER) {
			LOG(L_ERR, "rr:get_avp_cookies: not enough memory to prepare all cookies\n");
			goto brk;
		}
		memcpy(buf+len, &avp->flags, sizeof(char));
		len += sizeof(char);
		if (avp->flags & AVP_NAME_STR) {
			if (avp_name->len > 0xFFFF)
				ll = 0xFFFF;
			else
				ll = avp_name->len;
			memcpy(buf+len, &ll, sizeof(ll));
			len+= sizeof(ll);
			memcpy(buf+len, avp_name->s, ll);
			len+= ll;
		}
		else {
			memcpy(buf+len, &avp->id, sizeof(avp->id));
			len+= sizeof(avp->id);
		}
		if (avp->flags & AVP_VAL_STR) {
			if (avp_val.s.len > 0xFFFF)
				ll = 0xFFFF;
			else
				ll = avp_val.s.len;
			memcpy(buf+len, &ll, sizeof(ll));
			len+= sizeof(ll);
			memcpy(buf+len, avp_val.s.s, ll);
			len+= ll;
		}
		else {
			memcpy(buf+len, &avp_val.n, sizeof(avp_val.n));
			len+= sizeof(avp_val.n);
		}
	}
brk:
	if (len > sizeof(crc)) {
		result = (str*) pkg_malloc(sizeof(*result) + sizeof(crc) + (len*4)/3 + 3);
		if (!result) {
			LOG(L_ERR, "rr:get_avp_cookies: not enough memory\n");
			return 0;
		}
		result->s = (char*)result + sizeof(*result);
		crc = crcitt_string(buf+sizeof(crc), len-sizeof(crc));
		memcpy(&buf, &crc, sizeof(crc));

		base64encode(buf, len, result->s, &result->len);
		DBG("avp_cookies: len=%d, crc=0x%x, base64(%u)='%.*s'\n", len, crc, result->len, result->len, result->s);
	}
	return result;
}

void rr_set_avp_cookies(str *enc_cookies, regex_t *re) {
	char *buf;
	int len, pos;
	unsigned short crc;
	struct usr_avp avp;
	int_str avp_name, avp_val;
	regmatch_t pmatch;

	DBG("rr_set_avp_cookies: enc_cookie(%d)='%.*s'\n", enc_cookies->len, enc_cookies->len, enc_cookies->s);
	buf = (char*) pkg_malloc((enc_cookies->len*3)/4 + 3);
	if (!buf) {
		LOG(L_ERR, "rr:set_avp_cookies: not enough memory\n");
		return;
	}
	base64decode(enc_cookies->s, enc_cookies->len, buf, &len);

	if (len <= sizeof(crc))
		return;
	crc = crcitt_string(buf+sizeof(crc), len-sizeof(crc));
	if (crc != *(unsigned short*) buf) {
		LOG(L_ERR, "rr:set_avp_cookies: bad CRC when decoding AVP cookie\n");
		return;
	}
	pos = sizeof(crc);
	while (pos < len) {
		avp.flags = buf[pos];
		pos+= sizeof(char);
		if (avp.flags & AVP_NAME_STR) {
			avp_name.s.len = 0;
			memcpy(&avp_name.s.len, buf+pos, sizeof(unsigned short));
			avp_name.s.s = buf+pos+sizeof(unsigned short);
			pos+= sizeof(unsigned short)+avp_name.s.len;
			DBG("rr:set_avp_cookies: found cookie '%.*s'\n", avp_name.s.len, avp_name.s.s);
		}
		else {
			memcpy(&avp.id, buf+pos, sizeof(avp.id));
			pos+= sizeof(avp.id);
			avp_name.n = avp.id;
			DBG("rr:set_avp_cookies: found cookie #%d\n", avp_name.n);
		}
		if (pos >= len) {
			LOG(L_ERR, "rr:set_avp_cookies: AVP cookies corrupted\n");
			break;
		}
		if (avp.flags & AVP_VAL_STR) {
			avp_val.s.len = 0;
			memcpy(&avp_val.s.len, buf+pos, sizeof(unsigned short));
			avp_val.s.s = buf+pos+sizeof(unsigned short);
			pos+= sizeof(unsigned short)+avp_val.s.len;
		}
		else {
			memcpy(&avp_val.n, buf+pos, sizeof(avp_val.n));
			pos+= sizeof(avp_val.n);
		}
		if (pos > len) {
			LOG(L_ERR, "rr:set_avp_cookies: AVP cookies corrupted\n");
			break;
		}
		// filtr cookie
		if (re) {
			if (avp.flags & AVP_NAME_STR) {
				char savec;
				savec = avp_name.s.s[avp_name.s.len];
				avp_name.s.s[avp_name.s.len] = 0;
				if (regexec(re, avp_name.s.s, 1, &pmatch, 0) != 0) {
					DBG("rr:set_avp_cookies: regex doesn't match (str)\n");
					avp_name.s.s[avp_name.s.len] = savec;
					continue;
				}
				avp_name.s.s[avp_name.s.len] = savec;
			}
			else {
				char buf[25];
				snprintf(buf, sizeof(buf)-1, "i:%d", avp_name.n);
				buf[sizeof(buf)-1]=0;
				if (regexec(re, buf, 1, &pmatch, 0) != 0) {
					DBG("rr:set_avp_cookies: regex doesn't match (int)\n");
					continue;
				}
			}
		}
		// set avp from cookie
		DBG("rr:set_avp_cookies: adding AVP\n");

		if ( add_avp_list(&dialog_avp_list, avp.flags, avp_name, avp_val)!=0 ) {
			LOG(L_ERR, "ERROR: rr:set_avp_cookies: add_avp failed\n");
		}
	}
	pkg_free(buf);
}