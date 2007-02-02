/*
 * $Id: tls_init.c,v 1.8 2007/02/02 15:17:52 andrei Exp $
 *
 * TLS module - OpenSSL initialization funtions
 *
 * Copyright (C) 2001-2003 FhG FOKUS
 * Copyright (C) 2004,2005 Free Software Foundation, Inc.
 * Copyright (C) 2005,2006 iptelorg GmbH
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
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <string.h>
#include <openssl/ssl.h>
 
#include "../../dprint.h"
#include "../../mem/shm_mem.h"
#include "../../tcp_init.h"
#include "../../socket_info.h"
#include "../../pt.h"
#include "tls_verify.h"
#include "tls_domain.h"
#include "tls_util.h"
#include "tls_mod.h"
#include "tls_init.h"
#include "tls_locking.h"

#if OPENSSL_VERSION_NUMBER < 0x00907000L
#    warning ""
#    warning "==============================================================="
#    warning " Your version of OpenSSL is < 0.9.7."
#    warning " Upgrade for better compatibility, features and security fixes!"
#    warning "==============================================================="
#    warning ""
#endif

#if OPENSSL_VERSION_NUMBER >= 0x00908000L  /* 0.9.8*/
#    ifndef OPENSSL_NO_COMP
#        warning "openssl zlib compression bug workarround enabled"
#    endif
#    define TLS_FIX_ZLIB_COMPRESSION
#    include "fixed_c_zlib.h"
#endif

#ifdef TLS_KSSL_WORKARROUND

#	warning "openssl lib compiled with kerberos support which introduces a bug\
 (wrong malloc/free used in kssl.c) -- attempting workarround"
#	warning "NOTE: if you don't link libssl staticaly don't try running the \
compiled code on a system with a differently compiled openssl (it's safer \
to compile on the  _target_ system)"

#endif



#ifndef OPENSSL_NO_COMP
#define TLS_COMP_SUPPORT
#else
#undef TLS_COMP_SUPPORT
#endif

#ifndef OPENSSL_NO_KRB5
#define TLS_KERBEROS_SUPPORT
#else
#undef TLS_KERBEROS_SUPPORT
#endif


int tls_disable_compression = 0; /* by default enabled */
int tls_force_run = 0; /* ignore some start-up sanity checks, use it
						  at your own risk */

const SSL_METHOD* ssl_methods[TLS_USE_SSLv23 + 1];


/*
 * Wrappers around SER shared memory functions
 * (which can be macros)
 */
static void* ser_malloc(size_t size)
{
	return shm_malloc(size);
}


static void* ser_realloc(void *ptr, size_t size)
{
	return shm_realloc(ptr, size);
}


static void ser_free(void *ptr)
{
	shm_free(ptr);
}


/*
 * Initialize TLS socket
 */
int tls_init(struct socket_info *si)
{
	int ret;
	     /*
	      * reuse tcp initialization 
	      */
	ret = tcp_init(si);
	if (ret != 0) {
		ERR("Error while initializing TCP part of TLS socket %.*s:%d\n",
		    si->address_str.len, si->address_str.s, si->port_no);
		goto error;
	}
	
	si->proto = PROTO_TLS;
	return 0;
	
 error:
	if (si->socket != -1) {
		close(si->socket);
		si->socket = -1;
	}
	return ret;
}



/*
 * initialize ssl methods 
 */
static void init_ssl_methods(void)
{
	ssl_methods[TLS_USE_SSLv2_cli - 1] = SSLv2_client_method();
	ssl_methods[TLS_USE_SSLv2_srv - 1] = SSLv2_server_method();
	ssl_methods[TLS_USE_SSLv2 - 1] = SSLv2_method();
	
	ssl_methods[TLS_USE_SSLv3_cli - 1] = SSLv3_client_method();
	ssl_methods[TLS_USE_SSLv3_srv - 1] = SSLv3_server_method();
	ssl_methods[TLS_USE_SSLv3 - 1] = SSLv3_method();
	
	ssl_methods[TLS_USE_TLSv1_cli - 1] = TLSv1_client_method();
	ssl_methods[TLS_USE_TLSv1_srv - 1] = TLSv1_server_method();
	ssl_methods[TLS_USE_TLSv1 - 1] = TLSv1_method();
	
	ssl_methods[TLS_USE_SSLv23_cli - 1] = SSLv23_client_method();
	ssl_methods[TLS_USE_SSLv23_srv - 1] = SSLv23_server_method();
	ssl_methods[TLS_USE_SSLv23 - 1] = SSLv23_method();
}


/*
 * Fix openssl compression bugs if necessary
 */
static int init_tls_compression(void)
{
#if OPENSSL_VERSION_NUMBER >= 0x00908000L
	int n, r;
	STACK_OF(SSL_COMP)* comp_methods;
	SSL_COMP* zlib_comp;
	long ssl_version;
	
	/* disabling compression */
#	ifndef SSL_COMP_ZLIB_IDX
#		define SSL_COMP_ZLIB_IDX 1 /* openssl/ssl/ssl_ciph.c:84 */
#	endif 
	comp_methods = SSL_COMP_get_compression_methods();
	if (comp_methods == 0) {
		LOG(L_INFO, "tls: init_tls: compression support disabled in the"
					" openssl lib\n");
		goto end; /* nothing to do, exit */
	} else if (tls_disable_compression){
		LOG(L_INFO, "tls: init_tls: disabling compression...\n");
		sk_SSL_COMP_zero(comp_methods);
	}else{
		ssl_version=SSLeay();
		if (ssl_version >= 0x00908000L){
			/* the above SSL_COMP_get_compression_methods() call has the side
			 * effect of initializing the compression stack (if not already
			 * initialized) => after it zlib is initialized and in the stack */
			/* find zlib_comp (cannot use ssl3_comp_find, not exported) */
			n = sk_SSL_COMP_num(comp_methods);
			zlib_comp = 0;
			for (r = 0; r < n; r++) {
				zlib_comp = sk_SSL_COMP_value(comp_methods, r);
				DBG("tls: init_tls: found compression method %p id %d\n",
						zlib_comp, zlib_comp->id);
				if (zlib_comp->id == SSL_COMP_ZLIB_IDX) {
					DBG("tls: init_tls: found zlib compression (%d)\n", 
							SSL_COMP_ZLIB_IDX);
					break /* found */;
				} else {
					zlib_comp = 0;
				}
			}
			if (zlib_comp == 0) {
				LOG(L_INFO, "tls: init_tls: no openssl zlib compression "
							"found\n");
			}else{
				LOG(L_WARN, "tls: init_tls: detected openssl lib with "
							"known zlib compression bug: \"%s\" (0x%08lx)\n",
							SSLeay_version(SSLEAY_VERSION), ssl_version);
#	ifdef TLS_FIX_ZLIB_COMPRESSION
				LOG(L_WARN, "tls: init_tls: enabling openssl zlib compression "
							"bug workaround (replacing zlib COMP method with "
							"our own version)\n");
				/* hack: make sure that the CRYPTO_EX_INDEX_COMP class is empty
				 * and it does not contain any free_ex_data from the 
				 * built-in zlib. This can happen if the current openssl
				 * zlib malloc fix patch is used (CRYPTO_get_ex_new_index() in
				 * COMP_zlib()). Unfortunately the only way
				 * to do this it to cleanup all the ex_data stuff.
				 * It should be safe if this is executed before SSL_init()
				 * (only the COMP class is initialized before).
				 */
				CRYPTO_cleanup_all_ex_data();
				
				if (fixed_c_zlib_init() != 0) {
					LOG(L_CRIT, "tls: init_tls: BUG: failed to initialize zlib"
							" compression fix, disabling compression...\n");
					sk_SSL_COMP_zero(comp_methods); /* delete compression */
					goto end;
				}
				/* "fix" it */
				zlib_comp->method = &zlib_method;
#	else
				LOG(L_WARN, "tls: init_tls: disabling openssl zlib "
							"compression \n");
				zlib_comp=sk_SSL_COMP_delete(comp_methods, r);
				OPENSSL_free(zlib_comp);
#	endif
			}
		}
	}
end:
#endif /* OPENSSL_VERSION_NUMBER >= 0.9.8 */
	return 0;
}


/*
 * First step of TLS initialization
 */
int init_tls(void)
{
	struct socket_info* si;
	long ssl_version;
	int lib_kerberos;
	int lib_zlib;
	int kerberos_support;
	int comp_support;
	const char* lib_cflags;

#if OPENSSL_VERSION_NUMBER < 0x00907000L
	WARN("You are using an old version of OpenSSL (< 0.9.7). Upgrade!\n");
#endif
	ssl_version=SSLeay();
	/* check if version have the same major minor and fix level
	 * (e.g. 0.9.8a & 0.9.8c are ok, but 0.9.8 and 0.9.9x are not) */
	if ((ssl_version>>8)!=(OPENSSL_VERSION_NUMBER>>8)){
		LOG(L_CRIT, "ERROR: tls: tls_init: installed openssl library version "
				"is too different from the library the ser tls module was "
				"compiled with: installed \"%s\" (0x%08lx), compiled \"%s\" "
				"(0x%08lx).\n"
				" Please make sure a compatible version is used"
				" (tls_force_run in ser.cfg will override this check)\n",
				SSLeay_version(SSLEAY_VERSION), ssl_version,
				OPENSSL_VERSION_TEXT, (long)OPENSSL_VERSION_NUMBER);
		if (tls_force_run)
			LOG(L_WARN, "tls: tls_init: tls_force_run turned on, ignoring "
						" openssl version mismatch\n");
		else
			return -1; /* safer to exit */
	}
#ifdef TLS_KERBEROS_SUPPORT
	kerberos_support=1;
#else
	kerberos_support=0;
#endif
#ifdef TLS_COMP_SUPPORT
	comp_support=1;
#else
	comp_support=0;
#endif
	/* attempt to guess if the library was compiled with kerberos or
	 * compression support from the cflags */
	lib_cflags=SSLeay_version(SSLEAY_CFLAGS);
	lib_kerberos=0;
	lib_zlib=0;
	if ((lib_cflags==0) || strstr(lib_cflags, "not available")){ 
		lib_kerberos=-1;
		lib_zlib=-1;
	}else{
		if (strstr(lib_cflags, "-DZLIB"))
			lib_zlib=1;
		if (strstr(lib_cflags, "-DKRB5_"))
			lib_kerberos=1;
	}
	LOG(L_INFO, "tls: tls_init:  compiled  with  openssl  version " 
				"\"%s\" (0x%08lx), kerberos support: %s, compression: %s\n",
				OPENSSL_VERSION_TEXT, (long)OPENSSL_VERSION_NUMBER,
				kerberos_support?"on":"off", comp_support?"on":"off");
	LOG(L_INFO, "tls: tls_init: installed openssl library version "
				"\"%s\" (0x%08lx), kerberos support: %s, "
				" zlib compression: %s"
				"\n %s\n",
				SSLeay_version(SSLEAY_VERSION), ssl_version,
				(lib_kerberos==1)?"on":(lib_kerberos==0)?"off":"unkown",
				(lib_zlib==1)?"on":(lib_zlib==0)?"off":"unkown",
				SSLeay_version(SSLEAY_CFLAGS));
	if (lib_kerberos!=kerberos_support){
		if (lib_kerberos!=-1){
			LOG(L_CRIT, "ERROR: tls: tls_init: openssl compile options"
						" mismatch: library has kerberos support"
						" %s and ser tls %s (unstable configuration)\n"
						" (tls_force_run in ser.cfg will override this"
						" check)\n",
						lib_kerberos?"enabled":"disabled",
						kerberos_support?"enabled":"disabled"
				);
			if (tls_force_run)
				LOG(L_WARN, "tls: tls_init: tls_force_run turned on, ignoring "
						" kerberos support mismatch\n");
			else
				return -1; /* exit, is safer */
		}else{
			LOG(L_WARN, "WARNING: tls: tls_init: openssl  compile options"
						" missing -- cannot detect if kerberos support is"
						" enabled. Possible unstable configuration\n");
		}
	}
	     /*
	      * this has to be called before any function calling CRYPTO_malloc,
	      * CRYPTO_malloc will set allow_customize in openssl to 0 
	      */
	if (!CRYPTO_set_mem_functions(ser_malloc, ser_realloc, ser_free)) {
		ERR("Unable to set the memory allocation functions\n");
		return -1;
	}
	if (tls_init_locks()<0)
		return -1;
	init_tls_compression();
	#ifdef TLS_KSSL_WORKARROUND
		LOG(L_WARN, "tls: init_tls: openssl kerberos malloc bug detected, "
			" kerberos support will be disabled...\n");
	#endif
	SSL_library_init();
	SSL_load_error_strings();
	init_ssl_methods();

	     /* Now initialize TLS sockets */
	for(si = tls_listen; si; si = si->next) {
		if (tls_init(si) < 0)  return -1;
		     /* get first ipv4/ipv6 socket*/
		if ((si->address.af == AF_INET) &&
		    ((sendipv4_tls == 0) || (sendipv4_tls->flags & SI_IS_LO))) {
			sendipv4_tls = si;
		}
#ifdef USE_IPV6
		if ((sendipv6_tls == 0) && (si->address.af == AF_INET6)) {
			sendipv6_tls = si;
		}
#endif
	}

	return 0;
}


/*
 * Make sure that all server domains in the configuration have corresponding
 * listening socket in SER
 */
int tls_check_sockets(tls_cfg_t* cfg)
{
	tls_domain_t* d;

	if (!cfg) return 0;

	d = cfg->srv_list;
	while(d) {
		if (d->ip.len && !find_si(&d->ip, d->port, PROTO_TLS)) {
			ERR("%s: No listening socket found\n", tls_domain_str(d));
			return -1;
		}
		d = d->next;
	}
	return 0;
}


/*
 * TLS cleanup when SER exits
 */
void destroy_tls(void)
{
	ERR_free_strings();
	/* TODO: free all the ctx'es */
	tls_destroy_locks();
}
