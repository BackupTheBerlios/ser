/*
 * Copyright (C) 2001-2003 FhG Fokus
 * Copyright (C) 2004,2005 Free Software Foundation, Inc.
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

#ifndef tls_config_h
#define tls_config_h

#include "../tcp_conn.h"

#warning "====================================="
#warning ""
#warning "TLS code is still experimental."
#warning "Use at own risk, but use it :)"
#warning "Report bugs and experience to the development"
#warning "   lists of ser, mantainer and bug-tracking system."
#warning ""
#warning "====================================="

enum tls_method {
	TLS_METHOD_UNSPEC = 0,
	TLS_USE_SSLv2_cli,
	TLS_USE_SSLv2_srv,
	TLS_USE_SSLv2,
	TLS_USE_SSLv3_cli,
	TLS_USE_SSLv3_srv,
	TLS_USE_SSLv3,
	TLS_USE_TLSv1_cli,
	TLS_USE_TLSv1_srv,
	TLS_USE_TLSv1,
	TLS_USE_SSLv23_cli,
	TLS_USE_SSLv23_srv,
	TLS_USE_SSLv23
};

extern int      tls_log;
extern int      tls_method;

extern int      tls_verify_cert;
extern int      tls_require_cert;
extern char    *tls_cert_file;
extern char    *tls_pkey_file;
extern char    *tls_ca_file;
extern char    *tls_ciphers_list;
extern int      tls_handshake_timeout;
extern int      tls_send_timeout;

#endif
