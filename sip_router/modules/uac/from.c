/*
 * $Id: from.c,v 1.2 2005/03/18 20:02:39 ramona Exp $
 *
 * Copyright (C) 2005 Voice Sistem SRL
 *
 * This file is part of SIP Express Router.
 *
 * UAC SER-module is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * UAC SER-module is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * For any questions about this software and its license, please contact
 * Voice Sistem at following e-mail address:
 *         office@voice-sistem.ro
 *
 *
 * History:
 * ---------
 *  2005-01-31  first version (ramona)
 */


#include <ctype.h>

#include "../../parser/parse_from.h"
#include "../../mem/mem.h"
#include "../../data_lump.h"

#include "from.h"


extern str from_param;
extern int from_restore_mode;

static char enc_table64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz0123456789+/";

static int dec_table64[256];


#define text3B64_len(_l)   ( ( ((_l)+1)/3 ) << 2 )


void init_from_replacer()
{
	int i;

	for( i=0 ; i<256 ; i++)
		dec_table64[i] = -1;
	for ( i=0 ; i<64; i++)
		dec_table64[(unsigned char)enc_table64[i]] = i;
	}


static inline int encode_from( str *src, str *dst )
{
	static char buf[text3B64_len(MAX_URI_SIZE)];
	int  idx;
	int  left;
	int  block;
	int  i,r;
	char *p;

	dst->len = text3B64_len( src->len );
	dst->s = buf;
	if (dst->len>text3B64_len(MAX_URI_SIZE))
	{
		LOG(L_ERR,"ERROR:uac:encode_from: uri too long\n");
		return -1;
	}

	for ( idx=0, p=buf ; idx<src->len ; idx+=3)
	{
		left = src->len - idx - 1;
		left = (left>1? 2 : left);

		/* Collect 1 to 3 bytes to encode */
		block = 0;
		for ( i=0,r= 16 ; i<=left ; i++,r-=8 )
		{
			block += ((unsigned char)src->s[idx+i]) << r;
		}

		/* Encode into 2-4 chars appending '=' if not enough data left.*/
		*(p++) = enc_table64[(block >> 18) & 0x3f];
		*(p++) = enc_table64[(block >> 12) & 0x3f];
		*(p++) = left > 0 ? enc_table64[(block >> 6) & 0x3f] : '-';
		*(p++) = left > 1 ? enc_table64[block & 0x3f] : '-';
	}
	return 0;
}


static inline int decode_from( str *src , str *dst)
{
	static char buf[MAX_URI_SIZE];
	int block;
	int n;
	int idx;
	int end;
	int i,j;
	char c;

	/* Count '-' at end and disregard them */
	for( n=0,i=src->len-1; src->s[i]=='-'; i--)
		n++;

	dst->len = ((src->len * 6) >> 3) - n;
	dst->s = buf;
	if (dst->len>MAX_URI_SIZE)
	{
		LOG(L_ERR,"ERROR:uac:decode_from: uri too long\n");
		return -1;
	}

	end = src->len - n;
	for ( i=0,idx=0 ; i<end ; idx+=3 )
	{
		/* Assemble three bytes into an int from four "valid" characters */
		block = 0;
		for ( j=0; j<4 && i<end ; j++)
		{
			c = dec_table64[(unsigned char)src->s[i++]];
			if ( c<0 )
			{
				LOG(L_ERR,"ERROR:uac:decode_from: invalid base64 string "
					"\"%.*s\"\n",src->len,src->s);
				return -1;
			}
			block += c << (18 - 6*j);
		}

		/* Add the bytes */
		for ( j=0,n=16 ; j<3 && idx+j< dst->len; j++,n-=8 )
			buf[idx+j] = (char) ((block >> n) & 0xff);
	}

	return 0;

}


/* 
 * if display name does not exist, then from_dsp is ignored
 */
int replace_from( struct sip_msg *msg, str *from_dsp, str *from_uri)
{
	struct to_body *from;
	struct lump* l;
	str replace;
	char *p;
	str param;
	int offset;

	/* parse original from hdr */
	if (parse_from_header(msg)!=0 )
	{
		LOG(L_ERR,"ERROR:uac:replace_from: failed to find/parse FROM hdr\n");
		goto error;
	}
	from = (struct to_body*)msg->from->parsed;
	/* some validity checks */
	if (from->param_lst==0)
	{
		LOG(L_ERR,"ERROR:uac:replace_from: broken FROM hdr; no tag param\n");
		goto error;
	}

	/* first deal with display name */
	if (from_dsp && from->display.len)
	{
		/* must be replaced/ removed */
		l = 0;
		/* there is already a display -> remove it */
		DBG("DEBUG:uac:replace_from: removing display [%.*s]\n",
			from->display.len,from->display.s);
		/* build del lump */
		l = del_lump( msg, from->display.s-msg->buf, from->display.len, 0);
		if (l==0)
		{
			LOG(L_ERR,"ERROR:uac:replace_from: display del lump failed\n");
			goto error;
		}
		/* some new display to set? */
		if (from_dsp->s)
		{
			if (l==0)
			{
				/* add anchor just before uri's "<" */
				offset = from->uri.s - msg->buf;
				while( msg->buf[offset]!='<')
				{
					offset--;
					if (from->body.s>msg->buf+offset)
					{
						LOG(L_ERR,"ERROR:uac:replace_from: no <> and there"
								" is dispaly name\n");
						goto error;
					}
				}
				if ( (l=anchor_lump( msg, offset, 0, LUMP_ADD))==0)
				{
					LOG(L_ERR,"ERROR:uac:replace_from: display anchor lump "
						"failed\n");
					goto error;
				}
			}
			p = pkg_malloc( from_dsp->len);
			if (p==0)
			{
				LOG(L_ERR,"ERROR:uac:replace_from: no more pkg mem\n");
				goto error;
			}
			memcpy( p, from_dsp->s, from_dsp->len); 
			if (insert_new_lump_after( l, p, from_dsp->len, 0)==0)
			{
				LOG(L_ERR,"ERROR:uac:replace_from: insert new "
					"display lump failed\n");
				pkg_free(p);
				goto error;
			}
		}
	}

	/* now handle the URI */
	DBG("DEBUG:uac:replace_from: uri to replace [%.*s]\n",
		from->uri.len, from->uri.s);
	DBG("DEBUG:uac:replace_from: replacement uri is [%.*s]\n",
		from_uri->len, from_uri->s);

	/* build del/add lumps */
	if ((l=del_lump( msg, from->uri.s-msg->buf, from->uri.len, 0))==0)
	{
		LOG(L_ERR,"ERROR:uac:replace_from: del lump failed\n");
		goto error;
	}
	p = pkg_malloc( from_uri->len);
	if (p==0)
	{
		LOG(L_ERR,"ERROR:uac:replace_from: no more pkg mem\n");
		goto error;
	}
	memcpy( p, from_uri->s, from_uri->len); 
	if (insert_new_lump_after( l, p, from_uri->len, 0)==0)
	{
		LOG(L_ERR,"ERROR:uac:replace_from: insert new lump failed\n");
		pkg_free(p);
		goto error;
	}

	if (from_restore_mode==FROM_NO_RESTORE)
		return 0;

	/*add parameter lump */
	if (encode_from( &from->uri , &replace)<0 )
	{
		LOG(L_ERR,"ERROR:uac:replace_from: failed to encode uri\n");
		goto error;
	}
	DBG("encode is=<%.*s> len=%d\n",replace.len,replace.s,replace.len);
	offset = from->last_param->value.s+from->last_param->value.len-msg->buf;
	if ( (l=anchor_lump( msg, offset, 0, LUMP_ADD))==0)
	{
		LOG(L_ERR,"ERROR:uac:replace_from: anchor lump failed\n");
		goto error;
	}
	param.len = 1+from_param.len+1+replace.len;
	param.s = (char*)pkg_malloc(param.len);
	if (param.s==0)
	{
		LOG(L_ERR,"ERROR:uac:replace_from: no more pkg mem\n");
		goto error;
	}
	p = param.s;
	*(p++) = ';';
	memcpy( p, from_param.s, from_param.len);
	p += from_param.len;
	*(p++) = '=';
	memcpy( p, replace.s, replace.len);
	p += replace.len;
	if (insert_new_lump_after( l, param.s, param.len, 0)==0)
	{
		LOG(L_ERR,"ERROR:uac:replace_from: insert new lump failed\n");
		pkg_free(param.s);
		goto error;
	}

	return 0;
error:
	return -1;
}



int restore_from( struct sip_msg *msg, int is_req)
{
	struct to_body *ft_hdr;
	struct to_param *param;
	struct lump* l;
	str replace;
	str restore;
	str del;
	char *p;

	/* parse original from hdr */
	if (parse_from_header(msg)!=0 )
	{
		LOG(L_ERR,"ERROR:uac:restore_from: failed to find/parse FROM hdr\n");
		goto error;
	}
	ft_hdr = (struct to_body*)msg->from->parsed;

	/* check if it has the from param */
	for( param=ft_hdr->param_lst ; param ; param=param->next )
		if (param->name.len==from_param.len &&
		strncmp(param->name.s, from_param.s, from_param.len)==0)
			break;

	if (param==0)
	{
		if (!is_req)
			return 0;
		/*if request, check also TO hdr*/
		if ( !msg->to && (parse_headers(msg,HDR_TO_F,0)==-1 || !msg->to))
		{
			LOG(L_ERR,"ERROR:uac:restore_from: bad msg or missing TO hdr\n");
			goto error;
		}
		ft_hdr = (struct to_body*)msg->to->parsed;
		for( param=ft_hdr->param_lst ; param ; param=param->next )
			if (param->name.len==from_param.len &&
			strncmp(param->name.s, from_param.s, from_param.len)==0)
				break;
		if (param==0)
			return 0;
	}

	/* determin what to replace */
	replace.s = ft_hdr->uri.s;
	replace.len = ft_hdr->uri.len;
	DBG("DEBUG:uac:restore_from: replacing [%.*s]\n",
		replace.len, replace.s);

	/* build del/add lumps */
	if ((l=del_lump( msg, replace.s-msg->buf, replace.len, 0))==0)
	{
		LOG(L_ERR,"ERROR:uac:restore_from: del lump failed\n");
		goto error;
	}

	/* calculate the restore from */
	if (decode_from( &param->value, &restore)<0 )
	{
		LOG(L_ERR,"ERROR:uac:restore_from: failed to dencode uri\n");
		goto error;
	}
	DBG("DEBUG:uac:restore_from: replacement is [%.*s]\n",
		replace.len, replace.s);

	p = pkg_malloc( restore.len);
	if (p==0)
	{
		LOG(L_ERR,"ERROR:uac:restore_from: no more pkg mem\n");
		goto error;
	}
	memcpy( p, restore.s, restore.len);
	if (insert_new_lump_after( l, p, restore.len, 0)==0)
	{
		LOG(L_ERR,"ERROR:uac:restore_from: insert new lump failed\n");
		pkg_free(p);
		goto error;
	}

	/* delete parameter */
	del.s = param->name.s;
	while ( *del.s!=';')  del.s--;
	del.len = (int)(long)(param->value.s + param->value.len - del.s);
	DBG("DEBUG:uac:restore_from: deleting [%.*s]\n",del.len,del.s);
	if ((l=del_lump( msg, del.s-msg->buf, del.len, 0))==0) 
	{
		LOG(L_ERR,"ERROR:uac:restore_from: del lump failed\n");
		goto error;
	}

	return 0;
error:
	return -1;
}

