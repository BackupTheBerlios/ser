/*
 * $Id: uac.h,v 1.16 2003/01/27 21:19:48 jiri Exp $
 *
 *
 * Copyright (C) 2001-2003 Fhg Fokus
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


#ifndef _UAC_H
#define _UAC_H

#include "defs.h"


#include <stdio.h>
#include "config.h"
#include "t_dlg.h"

/* substitution character for FIFO UAC */
#define SUBST_CHAR '!'
#ifdef _DEPRECATED
/* number of random digits in beginning of a string --
   please multiples of 2 */
#define RAND_DIGITS	6
/* maximum seq size in hex chars */
#define MAX_SEQ_LEN (T_TABLE_POWER*2)
/* maximum size of pid in hex characters */
#define MAX_PID_LEN	4
extern char call_id[RAND_DIGITS+1+MAX_PID_LEN+1+MAX_SEQ_LEN+1];
void generate_callid();
#endif

#define DEFAULT_CSEQ	10

extern char *uac_from;
extern char *fifo;
extern int fifo_mode;

int uac_init();
int uac_child_init( int rank );

typedef int (*tuac_f)(str *msg_type, str *dst, str *headers,str *body,
	str *from, transaction_cb completion_cb, void *cbp,
	struct dialog *dlg );

typedef int (*tuacdlg_f)(str* msg_type, str* dst, str* ruri, str* to,
			 str* from, str* totag, str* fromtag, int* cseq,
			 str* callid, str* headers, str* body,
			 transaction_cb completion_cb, void* cbp
			 );

/* transactional UAC; look for an example of usage at fifo_uac */
int t_uac( 
	/* MESSAGE, OPTIONS, etc. */
	str *msg_type,  
	/* sip:foo@bar, will be put in r-uri and To */
	str *dst,	
	/* all other header fields separated by CRLF, including 
	   Content-type if body attached, excluding HFs
	   generated by UAC: To, Content_length, CSeq, Call-ID, Via, From
		(From is taken from config option)
	*/
	str *headers, 
	/* body of the message if any */
	str *body,
	str *from, /* optional value to be included in From *without* tag;
	              if 0, then config value uac_from will be used
	           */
	/* completion callback (optional) */
	transaction_cb completion_cb,
	/* callback parameter  -- it MUST be in shmem and it MAY NOT be
	   released -- TM does release the fragment; (actually, we should
	   do a favor to developers in cases like these and check the
	   pointer ranges -- good idea for a de-luxe release)
	*/
	void *cbp,
	struct dialog *dlg );


/* look at uac.c for usage guidelines */
/*
 * Send a request within a dialog
 */
int t_uac_dlg(str* msg,                     /* Type of the message - MESSAGE, OPTIONS etc. */
	      str* dst,                     /* Real destination (can be different than R-URI */
	      str* ruri,                    /* Request-URI */
	      str* to,                      /* To - including tag */
	      str* from,                    /* From - including tag */
	      str* totag,                   /* To tag */
	      str* fromtag,                 /* From tag */
	      int* cseq,                    /* CSeq */
	      str* cid,                     /* Call-ID */
	      str* headers,                 /* Optional headers including CRLF */
	      str* body,                    /* Message body */
	      transaction_cb completion_cb, /* Callback parameter */
	      void* cbp                     /* Callback pointer */
	      );

#ifndef DEPRECATE_OLD_STUFF
int fifo_uac( FILE *stream, char *response_file );
int fifo_uac_from( FILE *stream, char *response_file );
#endif


int fifo_uac_dlg( FILE *stream, char *response_file );


#endif
