/* $Id: fixed_c_zlib.h,v 1.1 2006/01/28 12:34:31 janakj Exp $
 * 
 * This file contains modified zlib compression functions
 * originally part of crypto/comp/c_zlib.c from the openssl library 
 * (version 0.9.8a).
 * It's distributed under the same license as OpenSSL.
 *
 * Copyright (c) 1998-2005 The OpenSSL Project.  All rights reserved.
 */
/*
 * The changes are: 
 *   - proper zalloc and zfree initialization for the zlib compression
 *     methods (use OPENSSL_malloc & OPENSSL_free to construct zalloc/zfree)
 *   - zlib_stateful_ex_idx is now a macro, a pointer to int is alloc'ed now
 *    on init and zlib_stateful_ex_idx is now the contents of this pointer 
 *    (deref). This allows using compression from different processes (if 
 *    the OPENSSL_malloc's are initialized previously to a shared mem. using
 *    version).
 *  -- andrei
 */


#ifdef TLS_FIX_ZLIB_COMPRESSION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/objects.h>
#include <openssl/comp.h>
#include <openssl/err.h>

#include <zlib.h>


/* alloc functions for zlib initialization */
static void* comp_calloc(void* foo, unsigned int no, unsigned int size)
{
	void *p;
	
	p=OPENSSL_malloc(no*size);
	if (p)
		memset(p, 0, no*size);
	return p;
}


/* alloc functions for zlib initialization */
static void comp_free(void* foo, void* p)
{
	OPENSSL_free(p);
}


static int zlib_stateful_init(COMP_CTX *ctx);
static void zlib_stateful_finish(COMP_CTX *ctx);
static int zlib_stateful_compress_block(COMP_CTX *ctx, unsigned char *out,
	unsigned int olen, unsigned char *in, unsigned int ilen);
static int zlib_stateful_expand_block(COMP_CTX *ctx, unsigned char *out,
	unsigned int olen, unsigned char *in, unsigned int ilen);


static COMP_METHOD zlib_method={
	NID_zlib_compression,
	LN_zlib_compression,
	zlib_stateful_init,
	zlib_stateful_finish,
	zlib_stateful_compress_block,
	zlib_stateful_expand_block,
	NULL,
	NULL,
	};


struct zlib_state
	{
	z_stream istream;
	z_stream ostream;
	};

static int* pzlib_stateful_ex_idx = 0; 
#define zlib_stateful_ex_idx (*pzlib_stateful_ex_idx)

int fixed_c_zlib_init()
{
	if (pzlib_stateful_ex_idx==0){
		if ((pzlib_stateful_ex_idx=OPENSSL_malloc(sizeof(int)))!=0){
			zlib_stateful_ex_idx=-1;
			return 0;
		} else return -1;
	}
	return -1;
}



static void zlib_stateful_free_ex_data(void *obj, void *item,
	CRYPTO_EX_DATA *ad, int ind,long argl, void *argp)
	{
	struct zlib_state *state = (struct zlib_state *)item;
	inflateEnd(&state->istream);
	deflateEnd(&state->ostream);
	OPENSSL_free(state);
	}

static int zlib_stateful_init(COMP_CTX *ctx)
	{
	int err;
	struct zlib_state *state =
		(struct zlib_state *)OPENSSL_malloc(sizeof(struct zlib_state));

	if (state == NULL)
		goto err;

	state->istream.zalloc = comp_calloc;
	state->istream.zfree = comp_free;
	state->istream.opaque = Z_NULL;
	state->istream.next_in = Z_NULL;
	state->istream.next_out = Z_NULL;
	state->istream.avail_in = 0;
	state->istream.avail_out = 0;
	err = inflateInit_(&state->istream,
		ZLIB_VERSION, sizeof(z_stream));
	if (err != Z_OK)
		goto err;

	state->ostream.zalloc = comp_calloc;
	state->ostream.zfree = comp_free;
	state->ostream.opaque = Z_NULL;
	state->ostream.next_in = Z_NULL;
	state->ostream.next_out = Z_NULL;
	state->ostream.avail_in = 0;
	state->ostream.avail_out = 0;
	err = deflateInit_(&state->ostream,Z_DEFAULT_COMPRESSION,
		ZLIB_VERSION, sizeof(z_stream));
	if (err != Z_OK)
		goto err;

	CRYPTO_new_ex_data(CRYPTO_EX_INDEX_COMP,ctx,&ctx->ex_data);
	if (zlib_stateful_ex_idx == -1)
		{
		CRYPTO_w_lock(CRYPTO_LOCK_COMP);
		if (zlib_stateful_ex_idx == -1)
			zlib_stateful_ex_idx =
				CRYPTO_get_ex_new_index(CRYPTO_EX_INDEX_COMP,
					0,NULL,NULL,NULL,zlib_stateful_free_ex_data);
		CRYPTO_w_unlock(CRYPTO_LOCK_COMP);
		if (zlib_stateful_ex_idx == -1)
			goto err;
		}
	CRYPTO_set_ex_data(&ctx->ex_data,zlib_stateful_ex_idx,state);
	return 1;
 err:
	if (state) OPENSSL_free(state);
	return 0;
	}

static void zlib_stateful_finish(COMP_CTX *ctx)
	{
	CRYPTO_free_ex_data(CRYPTO_EX_INDEX_COMP,ctx,&ctx->ex_data);
	}

static int zlib_stateful_compress_block(COMP_CTX *ctx, unsigned char *out,
	unsigned int olen, unsigned char *in, unsigned int ilen)
	{
	int err = Z_OK;
	struct zlib_state *state =
		(struct zlib_state *)CRYPTO_get_ex_data(&ctx->ex_data,
			zlib_stateful_ex_idx);

	if (state == NULL)
		return -1;

	state->ostream.next_in = in;
	state->ostream.avail_in = ilen;
	state->ostream.next_out = out;
	state->ostream.avail_out = olen;
	if (ilen > 0)
		err = deflate(&state->ostream, Z_SYNC_FLUSH);
	if (err != Z_OK)
		return -1;
#ifdef DEBUG_ZLIB
	fprintf(stderr,"compress(%4d)->%4d %s\n",
		ilen,olen - state->ostream.avail_out,
		(ilen != olen - state->ostream.avail_out)?"zlib":"clear");
#endif
	return olen - state->ostream.avail_out;
	}

static int zlib_stateful_expand_block(COMP_CTX *ctx, unsigned char *out,
	unsigned int olen, unsigned char *in, unsigned int ilen)
	{
	int err = Z_OK;

	struct zlib_state *state =
		(struct zlib_state *)CRYPTO_get_ex_data(&ctx->ex_data,
			zlib_stateful_ex_idx);

	if (state == NULL)
		return 0;

	state->istream.next_in = in;
	state->istream.avail_in = ilen;
	state->istream.next_out = out;
	state->istream.avail_out = olen;
	if (ilen > 0)
		err = inflate(&state->istream, Z_SYNC_FLUSH);
	if (err != Z_OK)
		return -1;
#ifdef DEBUG_ZLIB
	fprintf(stderr,"expand(%4d)->%4d %s\n",
		ilen,olen - state->istream.avail_out,
		(ilen != olen - state->istream.avail_out)?"zlib":"clear");
#endif
	return olen - state->istream.avail_out;
	}

#endif /* TLS_FIX_ZLIB_COMPRESSION */