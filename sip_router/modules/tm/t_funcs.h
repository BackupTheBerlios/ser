/*
 * $Id: t_funcs.h,v 1.32 2002/03/01 22:23:53 bogdan Exp $
 */


#ifndef _T_FUNCS_H
#define _T_FUNCS_H

#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include "../../msg_parser.h"
#include "../../globals.h"
#include "../../udp_server.h"
#include "../../msg_translator.h"
#include "../../forward.h"
#include "../../mem/mem.h"

struct s_table;
struct timer;
struct entry;
struct cell;

extern struct cell         *T;
extern unsigned int     global_msg_id;
extern struct s_table*  hash_table;


#include "sh_malloc.h"

#include "timer.h"
#include "lock.h"
#include "sip_msg.h"


#define LOCK_REPLIES(_t) lock(&(_t)->reply_mutex )
#define UNLOCK_REPLIES(_t) unlock(&(_t)->reply_mutex )
#define LOCK_ACK(_t) lock(&(_t)->ack_mutex )
#define UNLOCK_ACK(_t) unlock(&(_t)->ack_mutex )
#define LOCK_WAIT(_t) lock(&(_t)->wait_mutex )
#define UNLOCK_WAIT(_t) unlock(&(_t)->wait_mutex )


/* convenience short-cut macros */
#define REQ_METHOD first_line.u.request.method_value
#define REPLY_STATUS first_line.u.reply.statuscode
#define REPLY_CLASS(_reply) ((_reply)->REPLY_STATUS/100)

/* send a private buffer: utilize a retransmission structure
   but take a separate buffer not refered by it; healthy
   for reducing time spend in REPLIES locks
*/

#define SEND_PR_BUFFER(_rb,_bf,_le ) ({ if ((_rb)->retr_buffer) \
	{ udp_send( (_bf), (_le), (struct sockaddr*)&((_rb)->to) , \
	   sizeof(struct sockaddr_in) ); \
	} else { \
	DBG("ERROR: attempt to send an empty buffer from %s (%d)", \
	__FUNCTION__, __LINE__ ); }})

#define SEND_BUFFER( _rb ) SEND_PR_BUFFER( \
	_rb,(_rb)->retr_buffer, (_rb)->bufflen )

/*
#define SEND_BUFFER( _rb ) ({ if ((_rb)->retr_buffer) \
	{ udp_send( (_rb)->retr_buffer, \
	  (_rb)->bufflen, (struct sockaddr*)&((_rb)->to) , \
	  sizeof(struct sockaddr_in) ); \
	} else \
	DBG("ERROR: attempt to send an empty buffer from %s (%d)", \
	__FUNCTION__, __LINE__ ); })
*/


/* 
  macros for reference bitmap (lock-less process non-exclusive ownership) 
*/
#define T_IS_REFED(_T_cell) ((_T_cell)->ref_bitmap)
#define T_REFCOUNTER(_T_cell) \
	( { int _i=0; \
		process_bm_t _b=(_T_cell)->ref_bitmap; \
		while (_b) { \
			if ( (_b) & 1 ) _i++; \
			(_b) >>= 1; \
		} ;\
		(_i); \
	 } )
		

#ifdef EXTRA_DEBUG
#define T_IS_REFED_BYSELF(_T_cell) ((_T_cell)->ref_bitmap & process_bit)
#	define DBG_REF(_action, _t) DBG("DEBUG: XXXXX %s (%s:%d): T=%p , ref (bm=%x, cnt=%d)\n",\
			(_action), __FUNCTION__, __LINE__, (_t),(_t)->ref_bitmap, T_REFCOUNTER(_t));
#	define T_UNREF(_T_cell) \
	( { \
		DBG_REF("unref", (_T_cell)); \
		if (!T_IS_REFED_BYSELF(_T_cell)) { \
			DBG("ERROR: unrefering unrefered transaction %p from %s , %s : %d\n", \
				(_T_cell), __FUNCTION__, __FILE__, __LINE__ ); \
			abort(); \
		} \
		(_T_cell)->ref_bitmap &= ~process_bit; \
	} )

#	define T_REF(_T_cell) \
	( { \
		DBG_REF("ref", (_T_cell));	 \
		if (T_IS_REFED_BYSELF(_T_cell)) { \
			DBG("ERROR: refering already refered transaction %p from %s,%s :"\
				" %d\n",(_T_cell), __FUNCTION__, __FILE__, __LINE__ ); \
			abort(); \
		} \
		(_T_cell)->ref_bitmap |= process_bit; \
	} )
#else
#	define T_UNREF(_T_cell) ({ (_T_cell)->ref_bitmap &= ~process_bit; })
#	define T_REF(_T_cell) ({ (_T_cell)->ref_bitmap |= process_bit; })
#endif


	

#ifdef _OLD_XX
#define unref_T(_T_cell) \
	( {\
		lock( &(hash_table->entrys[(_T_cell)->hash_index].mutex) );\
		(_T_cell)->ref_counter--;\
		DBG_REF("unref", (_T_cell)); \
		unlock( &(hash_table->entrys[(_T_cell)->hash_index].mutex) );\
	} );

/* we assume that ref_T is only called from places where
   the associated locks are set up and we don't need to
   lock/unlock
*/
#define ref_T(_T_cell) ({ ((_T_cell)->ref_counter++); \
		DBG_REF("ref", (_T_cell));	})
#endif

enum addifnew_status { AIN_ERROR, AIN_RETR, AIN_NEW, AIN_NEWACK, AIN_OLDACK } ;


int   tm_startup();
void tm_shutdown();


/* function returns:
 *       1 - a new transaction was created
 *      -1 - error, including retransmission
 */
int  t_add_transaction( struct sip_msg* p_msg  );




/* function returns:
 *      -1 - transaction wasn't found
 *       1 - transaction found
 */
int t_check( struct sip_msg* , int *branch , int* is_cancel);




/* Forwards the inbound request to a given IP and port.  Returns:
 *       1 - forward successfull
 *      -1 - error during forward
 */
int t_forward( struct sip_msg* p_msg , unsigned int dst_ip ,
										unsigned int dst_port);




/* Forwards the inbound request to dest. from via.  Returns:
 *       1 - forward successfull
 *      -1 - error during forward
 */
int t_forward_uri( struct sip_msg* p_msg  );



/* This function is called whenever a reply for our module is received;
 * we need to register this function on module initialization;
 * Returns :   0 - core router stops
 *             1 - core router relay statelessly
 */

int t_on_reply( struct sip_msg  *p_msg ) ;




/* This function is called whenever a request for our module is received;
 * we need to register this function on module initialization;
 * Returns :   0 - core router stops
 *             1 - core router relay statelessly
 */
int t_on_request_received( struct sip_msg  *p_msg , unsigned int ip, unsigned int port) ;




/* This function is called whenever a request for our module is received;
 * we need to register this function on module initialization;
 * Returns :   0 - core router stops
 *             1 - core router relay statelessly
 */
int t_on_request_received_uri( struct sip_msg  *p_msg ) ;




/* returns 1 if everything was OK or -1 for error
*/
int t_release_transaction( struct sip_msg* );




/* Retransmits the last sent inbound reply.
 * Returns  -1 - error
 *           1 - OK
 */
int t_retransmit_reply( /* struct sip_msg * */  );




/* Force a new response into inbound response buffer.
 * returns 1 if everything was OK or -1 for erro
 */
int t_send_reply( struct sip_msg * , unsigned int , char *  );




/* releases T-context */
int t_unref( /* struct sip_msg* p_msg */ );




int t_forward_nonack( struct sip_msg* p_msg , unsigned int dest_ip_param ,
	unsigned int dest_port_param );
int t_forward_ack( struct sip_msg* p_msg , unsigned int dest_ip_param ,
	unsigned int dest_port_param );
struct cell* t_lookupOriginalT(  struct s_table* hash_table,
	struct sip_msg* p_msg );
int t_reply_matching( struct sip_msg* , unsigned int* , unsigned int* );
int t_store_incoming_reply( struct cell* , unsigned int , struct sip_msg* );
int t_lookup_request( struct sip_msg* p_msg , int leave_new_locked );
int t_all_final( struct cell * );
int t_build_and_send_ACK( struct cell *Trans , unsigned int brach ,
	struct sip_msg* rpl);
int t_cancel_branch(unsigned int branch); //TO DO
int t_should_relay_response( struct cell *Trans, int new_code, int branch,
	int *should_store );
int t_update_timers_after_sending_reply( struct retrans_buff *rb );
int t_put_on_wait(  struct cell  *Trans  );
int relay_lowest_reply_upstream( struct cell *Trans , struct sip_msg *p_msg );
static int push_reply( struct cell* trans , unsigned int branch ,
	char *buf, unsigned int len);
int add_branch_label( struct cell *Trans, struct sip_msg *p_msg , int branch );
int get_ip_and_port_from_uri( struct sip_msg* p_msg , unsigned int *param_ip,
	unsigned int *param_port);
struct retrans_buff *build_ack( struct sip_msg* rpl, struct cell *trans,
	int branch );
enum addifnew_status t_addifnew( struct sip_msg* p_msg );




inline int static attach_ack(  struct cell *t, int branch,
    struct retrans_buff *srb )
{
	LOCK_ACK( t );
	if (t->outbound_ack[branch]) {
		UNLOCK_ACK(t);
		shm_free( srb );
		LOG(L_WARN, "attach_ack: Warning: ACK already sent out\n");
		return 0;
	}
	t->outbound_ack[branch] = srb;
	UNLOCK_ACK( t );
	return 1;
}




inline int static relay_ack( struct cell *t, int branch, 
	struct retrans_buff *srb, int len )
{
	memset( srb, 0, sizeof( struct retrans_buff ) );
	memcpy( & srb->to, & t->ack_to, sizeof (struct sockaddr_in));
	srb->tolen = sizeof (struct sockaddr_in);
	srb->my_T = t;
	srb->branch = branch;
	srb->retr_buffer = (char *) srb + sizeof( struct retrans_buff );
	srb->bufflen = len;
	SEND_BUFFER( srb );
	return attach_ack( t, branch, srb );
}


#endif
