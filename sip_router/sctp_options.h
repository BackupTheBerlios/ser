/* 
 * $Id: sctp_options.h,v 1.1 2008/08/08 20:47:23 andrei Exp $
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
 */

#ifndef _sctp_options_h
#define _sctp_options_h

#define DEFAULT_SCTP_AUTOCLOSE 180 /* seconds */
#define DEFAULT_SCTP_SEND_TTL  32000 /* in ms (32s)  */


struct sctp_cfg_options{
	int sctp_so_rcvbuf;
	int sctp_so_sndbuf;
	unsigned int sctp_autoclose; /* in seconds */
	unsigned int sctp_send_ttl; /* in milliseconds */
};

extern struct sctp_cfg_options sctp_options;

void init_sctp_options();
void sctp_options_check();
void sctp_options_get(struct sctp_cfg_options *s);

#endif /* _sctp_options_h */
