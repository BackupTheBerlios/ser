/*
 * $Id: timer.h,v 1.21 2001/12/10 15:42:45 bogdan Exp $
 */


#ifndef _TIMER_H
#define _TIMER_H

enum lists { RETRASMISSIONS_LIST, FR_TIMER_LIST, WT_TIMER_LIST, DELETE_LIST, NR_OF_TIMER_LISTS };

/* we maintain separate retransmission lists for each of retransmission
   periods; that allows us to keep the lists ordered while just adding
   new items to list's tail (FIFO)
*/
//enum retransmission_lists { RT_T1_TO1, RT_T1_TO_2, RT_T1_TO_3, RT_T2, NR_OF_RT_LISTS };


/* FINAL_RESPONSE_TIMER ... tells how long should the transaction engine
   wait if no final response comes back*/
#define FR_TIME_OUT            16
#define INV_FR_TIME_OUT     30

/* WAIT timer ... tells how long state should persist in memory after
   a transaction was finalized*/
#define WT_TIME_OUT      5

/* DELETE timer ... tells how long should the transaction persist in memory
   after it was removed from the hash table and before it will be deleted */
#define DEL_TIME_OUT      2


#define RETR_T1  1
#define RETR_T2  4


#define is_in_timer_list(tl,id)  \
             (tl->next_tl || tl->prev_tl || (!tl->next_tl && !tl->prev_tl && tl==hash_table->timers[id].first_tl) )


/* all you need to put a cell in a timer list:
   links to neighbours and timer value         */
typedef struct timer_link
{
   struct timer_link *next_tl;
   struct timer_link *prev_tl;
   unsigned int        time_out;
   void                      *payload;
}timer_link_type ;

#include "h_table.h"


void                        add_to_tail_of_timer_list( struct s_table* hash_table , struct timer_link * tl , int list_id, unsigned int time_out );
void                        insert_into_timer_list( struct s_table* hash_table , struct timer_link* tl, int list_id , unsigned int time_out );
void                        remove_from_timer_list( struct s_table* hash_table , struct timer_link* tl , int list_id);
void                        timer_routine(unsigned int, void *);


#endif
