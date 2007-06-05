/*
 * $Id: t_cancel.c,v 1.26 2007/06/05 15:16:44 andrei Exp $
 *
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
 *
 * History:
 * ----------
 * 2003-04-14  checking if a reply sent before cancel is initiated
 *             moved here (jiri)
 * 2004-02-11  FIFO/CANCEL + alignments (hash=f(callid,cseq)) (uli+jiri)
 * 2004-02-13  timer_link.payload removed (bogdan)
 * 2006-10-10  cancel_uacs  & cancel_branch take more options now (andrei)
 * 2007-03-15  TMCB_ONSEND hooks added (andrei)
 * 2007-05-28: cancel_branch() constructs the CANCEL from the
 *             outgoing INVITE instead of the incomming one.
 *             (it can be disabled with reparse_invite=0) (Miklos)
 * 2007-06-04  cancel_branch() can operate in lockless mode (with a lockless
 *              should_cancel()) (andrei)
 */

#include <stdio.h> /* for FILE* in fifo_uac_cancel */

#include "defs.h"


#include "t_funcs.h"
#include "../../dprint.h"
#include "../../ut.h"
#include "t_reply.h"
#include "t_cancel.h"
#include "t_msgbuilder.h"
#include "t_lookup.h" /* for t_lookup_callid in fifo_uac_cancel */
#include "t_hooks.h"


/* determine which branches should be canceled; can be called 
   without REPLY_LOCK, since should_cancel_branch() is atomic now
   -- andrei
 WARNING: - has side effects, see should_cancel_branch()
          - one _must_ call cancel_uacs(cancel_bm) if *cancel_bm!=0 or
            you'll have some un-cancelable branches */
void which_cancel( struct cell *t, branch_bm_t *cancel_bm )
{
	int i;
	int branches_no;
	
	*cancel_bm=0;
	branches_no=t->nr_of_outgoings;
	membar_depends(); 
	for( i=0 ; i<branches_no ; i++ ) {
		if (should_cancel_branch(t, i, 1)) 
			*cancel_bm |= 1<<i ;
	}
}




/* cancel branches scheduled for deletion
 * params: t          - transaction
 *          cancel_bm - bitmap with the branches that are supposed to be 
 *                       canceled 
 *          flags     - how_to_cancel flags, see cancel_branch()
 * returns: bitmap with the still active branches (on fr timer)
 * WARNING: always fill cancel_bm using which_cancel(), supplying values
 *          in any other way is a bug*/
int cancel_uacs( struct cell *t, branch_bm_t cancel_bm, int flags)
{
	int i;
	int ret;
	int r;

	ret=0;
	/* cancel pending client transactions, if any */
	for( i=0 ; i<t->nr_of_outgoings ; i++ ) 
		if (cancel_bm & (1<<i)){
			r=cancel_branch(t, i, flags);
			ret|=(r!=0)<<i;
		}
	return ret;
}



/* should be called directly only if one of the condition bellow is true:
 *  - should_cancel_branch or which_cancel returned true for this branch
 *  - buffer value was 0 and then set to BUSY in an atomic op.:
 *     if (atomic_cmpxchg_long(&buffer, 0, BUSY_BUFFER)==0).
 *
 * params:  t - transaction
 *          branch - branch number to be canceled
 *          flags - howto cancel: 
 *                   F_CANCEL_B_KILL - will completely stop the 
 *                     branch (stops the timers), use with care
 *                   F_CANCEL_B_FAKE_REPLY - will send a fake 487
 *                      to all branches that haven't received any response
 *                      (>=100). It assumes the REPLY_LOCK is not held
 *                      (if it is => deadlock)
 *                  F_CANCEL_B_FORCE - will send a cancel (and create the 
 *                       corresp. local cancel rb) even if no reply was 
 *                       received; F_CANCEL_B_FAKE_REPLY will be ignored.
 *                  default: stop only the retransmissions for the branch
 *                      and leave it to timeout if it doesn't receive any
 *                      response to the CANCEL
 * returns: 0 - branch inactive after running cancel_branch() 
 *          1 - branch still active  (fr_timer)
 *         -1 - error
 * WARNING:
 *          - F_CANCEL_KILL_B should be used only if the transaction is killed
 *            explicitly afterwards (since it might kill all the timers
 *            the transaction won't be able to "kill" itself => if not
 *            explicitly "put_on_wait" it might live forever)
 *          - F_CANCEL_B_FAKE_REPLY must be used only if the REPLY_LOCK is not
 *            held
 *          - checking for buffer==0 under REPLY_LOCK is no enough, an 
 *           atomic_cmpxhcg or atomic_get_and_set _must_ be used.
 */
int cancel_branch( struct cell *t, int branch, int flags )
{
	char *cancel;
	unsigned int len;
	struct retr_buf *crb, *irb;
	int ret;
	branch_bm_t tmp_bm;

	crb=&t->uac[branch].local_cancel;
	irb=&t->uac[branch].request;
	irb->flags|=F_RB_CANCELED;
	ret=1;

#	ifdef EXTRA_DEBUG
	if (crb->buffer!=BUSY_BUFFER) {
		LOG(L_CRIT, "ERROR: attempt to rewrite cancel buffer: %p\n",
				crb->buffer);
		abort();
	}
#	endif

	if (flags & F_CANCEL_B_KILL){
		stop_rb_timers( irb );
		ret=0;
		if ((t->uac[branch].last_received < 100) &&
				!(flags & F_CANCEL_B_FORCE)) {
			DBG("DEBUG: cancel_branch: no response ever received: "
			    "giving up on cancel\n");
			/* remove BUSY_BUFFER -- mark cancel buffer as not used */
			atomic_set_long((void*)&crb->buffer, 0);
			if (flags & F_CANCEL_B_FAKE_REPLY){
				LOCK_REPLIES(t);
				if (relay_reply(t, FAKED_REPLY, branch, 487, &tmp_bm) == 
										RPS_ERROR){
					return -1;
				}
			}
			/* do nothing, hope that the caller will clean up */
			return ret;
		}
	}else{
		stop_rb_retr(irb); /* stop retransmissions */
		if ((t->uac[branch].last_received < 100) &&
				!(flags & F_CANCEL_B_FORCE)) {
			/* no response received => don't send a cancel on this branch,
			 *  just drop it */
			/* remove BUSY_BUFFER -- mark cancel buffer as not used */
			atomic_set_long((void*)&crb->buffer, 0);
			if (flags & F_CANCEL_B_FAKE_REPLY){
				stop_rb_timers( irb ); /* stop even the fr timer */
				LOCK_REPLIES(t);
				if (relay_reply(t, FAKED_REPLY, branch, 487, &tmp_bm) == 
										RPS_ERROR){
					return -1;
				}
				return 0; /* should be inactive after the 487 */
			}
			/* do nothing, just wait for the final timeout */
			return 1;
		}
	}

	if (reparse_invite) {
		/* build the CANCEL from the INVITE which was sent out */
		cancel = build_local_reparse(t, branch, &len, CANCEL, CANCEL_LEN, &t->to);
	} else {
		/* build the CANCEL from the reveived INVITE */
		cancel = build_local(t, branch, &len, CANCEL, CANCEL_LEN, &t->to);
	}
	if (!cancel) {
		LOG(L_ERR, "ERROR: attempt to build a CANCEL failed\n");
		/* remove BUSY_BUFFER -- mark cancel buffer as not used */
		atomic_set_long((void*)&crb->buffer, 0);
		return -1;
	}
	/* install cancel now */
	crb->dst = irb->dst;
	crb->branch = branch;
	/* label it as cancel so that FR timer can better know how to
	   deal with it */
	crb->activ_type = TYPE_LOCAL_CANCEL;
	/* be extra carefully and check for bugs (the below if could be replaced
	 *  by an atomic_set((void*)&crb->buffer, cancel) */
	if (unlikely(atomic_cmpxchg_long((void*)&crb->buffer, (long)BUSY_BUFFER,
							(long)cancel)!= (long)BUSY_BUFFER)){
		BUG("tm: cancel_branch: local_cancel buffer=%p != BUSY_BUFFER"
				" (trying to continue)\n", crb->buffer);
		shm_free(cancel);
		return -1;
	}
	membar_write_atomic_op(); /* cancel retr. can be called from 
								 reply_received w/o the reply lock held => 
								 they check for buffer_len to 
								 see if a valid reply exists */
	crb->buffer_len = len;

	DBG("DEBUG: cancel_branch: sending cancel...\n");
#ifdef TMCB_ONSEND
	if (SEND_BUFFER( crb )>=0){
		if (unlikely (has_tran_tmcbs(t, TMCB_REQUEST_SENT)))
			run_onsend_callbacks(TMCB_REQUEST_SENT, crb, 0, 0, TMCB_LOCAL_F);
	}
#else
	SEND_BUFFER( crb );
#endif
	/*sets and starts the FINAL RESPONSE timer */
	if (start_retr( crb )!=0)
		LOG(L_CRIT, "BUG: cancel_branch: failed to start retransmission"
					" for %p\n", crb);
	return ret;
}


const char* rpc_cancel_doc[2] = {
	"Cancel a pending transaction",
	0
};


/* fifo command to cancel a pending call (Uli)
 * Syntax:
 *
 * ":uac_cancel:[response file]\n
 * callid\n
 * cseq\n
 */
void rpc_cancel(rpc_t* rpc, void* c)
{
	struct cell *trans;
	static char cseq[128], callid[128];
	branch_bm_t cancel_bm;
	int i,j;

	str cseq_s;   /* cseq */
	str callid_s; /* callid */

	cseq_s.s=cseq;
	callid_s.s=callid;
	cancel_bm=0;

	if (rpc->scan(c, "SS", &callid_s, &cseq_s) < 2) {
		rpc->fault(c, 400, "Callid and CSeq expected as parameters");
		return;
	}

	if( t_lookup_callid(&trans, callid_s, cseq_s) < 0 ) {
		DBG("Lookup failed\n");
		rpc->fault(c, 400, "Transaction not found");
		return;
	}
	/*  find the branches that need cancel-ing */
	LOCK_REPLIES(trans);
		which_cancel(trans, &cancel_bm);
	UNLOCK_REPLIES(trans);
	 /* tell tm to cancel the call */
	DBG("Now calling cancel_uacs\n");
	i=cancel_uacs(trans, cancel_bm, 0); /* don't fake 487s, 
										 just wait for timeout */
	
	/* t_lookup_callid REF`d the transaction for us, we must UNREF here! */
	UNREF(trans);
	j=0;
	while(i){
		j++;
		i&=i-1;
	}
	rpc->add(c, "ds", j, "branches remaining (waiting for timeout)");
}
