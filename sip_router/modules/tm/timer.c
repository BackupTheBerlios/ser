#include "timer.h"
#include "../../dprint.h"


/* put a new cell into a list nr. list_id within a hash_table;
  * set initial timeout
  */
void add_to_tail_of_timer_list( struct s_table* hash_table , struct timer_link* tl, int list_id , unsigned int time_out )
{
   struct timer* timer_list = &(hash_table->timers[ list_id ]);

   tl->time_out = time_out + hash_table->time;
   tl->next_tl= 0;
   DBG("DEBUG: add_to_tail_of_timer[%d]: %d, %p\n",list_id,tl->time_out,tl);

   /* the entire timer list is locked now -- noone else can manipulate it */
   lock( timer_list->mutex );
   if (timer_list->last_tl)
   {
       tl->prev_tl=timer_list->last_tl;
       timer_list->last_tl->next_tl = tl;
       timer_list->last_tl = tl ;
   } else {
       tl->prev_tl = 0;
       timer_list->first_tl = tl;
       timer_list->last_tl = tl;
   }
   /* give the list lock away */
   unlock( timer_list->mutex );
}




/*
  */
void insert_into_timer_list( struct s_table* hash_table , struct timer_link* new_tl, int list_id , unsigned int time_out )
{
   struct timer          *timer_list = &(hash_table->timers[ list_id ]);
   struct timer_link  *tl;

   new_tl->time_out = time_out + hash_table->time;
   DBG("DEBUG: insert_into_timer[%d]: %d, %p\n",list_id,new_tl->time_out,new_tl);

    /* if we have an empty list*/
   if ( !timer_list->first_tl )
   {
      new_tl->next_tl= 0;
      new_tl->prev_tl = 0;
      lock( timer_list->mutex );
      timer_list->first_tl = new_tl;
      timer_list->last_tl = new_tl;
      unlock( timer_list->mutex );
      return;
   }

   for( tl=timer_list->first_tl ; tl && tl->time_out<new_tl->time_out ; tl=tl->next_tl );

   lock( timer_list->mutex );
   if ( tl )
   {
      /* insert before tl*/
      new_tl->prev_tl = tl->prev_tl;
      tl->prev_tl = new_tl;
      if ( new_tl->prev_tl )
         new_tl->prev_tl->next_tl = new_tl;
      else
         timer_list->first_tl = new_tl;
      new_tl->next_tl = tl;
   }
   else
   {
      /* insert at the end */
      new_tl->next_tl = 0;
      new_tl->prev_tl = timer_list->last_tl;
      timer_list->last_tl->next_tl = new_tl;
      timer_list->last_tl = new_tl ;
    }
    unlock( timer_list->mutex );
}




/* remove a cell from a list nr. list_id within a hash_table;
*/
void remove_from_timer_list( struct s_table* hash_table , struct timer_link* tl , int list_id)
{
   struct timer* timers=&(hash_table->timers[ list_id ]);
   DBG("DEBUG: remove_from_timer[%d]: %d, %p \n",list_id,tl->time_out,tl);

   if (tl->next_tl || tl->prev_tl || (!tl->next_tl && !tl->prev_tl && tl==timers->first_tl)   )
   {
      lock( timers->mutex );
      if ( tl->prev_tl )
         tl->prev_tl->next_tl = tl->next_tl;
      else
         timers->first_tl = tl->next_tl;
      if ( tl->next_tl )
         tl->next_tl->prev_tl = tl->prev_tl;
      else
         timers->last_tl = tl->prev_tl;
      unlock( timers->mutex );
      tl->next_tl = 0;
      tl->prev_tl = 0;
   }
}




/* remove a cell from the head of  list nr. list_id within a hash_table;
*/
struct timer_link  *remove_from_timer_list_from_head( struct s_table* hash_table, int list_id )
{
   struct timer* timers=&(hash_table->timers[ list_id ]);
   struct timer_link *tl = timers->first_tl;

   if  (tl)
   {
      DBG("DEBUG: remove_from_timer_head[%d]: %d , p=%p , next=%p\n",list_id,tl->time_out,tl,tl->next_tl);
      lock( timers->mutex  );
      timers->first_tl = tl->next_tl;
      if (!timers->first_tl)
         timers->last_tl=0;
      else
         tl->next_tl->prev_tl = 0;
      unlock( timers->mutex );
      tl->next_tl = 0;
      tl->prev_tl = 0;
   }
   else
      DBG("DEBUG: remove_from_timer_head[%d]: list is empty! nothing to remove!\n",list_id);


   return tl;
}





void * timer_routine(void * attr)
{
   struct s_table      *hash_table = (struct s_table *)attr;
   struct timer*         timers= hash_table->timers;
   struct timeval       a_sec;
   struct timer_link* tl;
   int unsigned*         time =  &(hash_table->time);
   int                           id;


   while (1)
   {
      a_sec.tv_sec   = 1;
      a_sec.tv_usec = 0;
      select( 0 , 0 , 0 ,0 , &a_sec );
      (*time)++;
      DBG("%d\n", *time);

      for( id=0 ; id<NR_OF_TIMER_LISTS ; id++ )
         while ( timers[ id ].first_tl && timers[ id ].first_tl->time_out <= *time )
         {
            tl = remove_from_timer_list_from_head( hash_table, id );
            timers[id].timeout_handler( tl->payload );
         }
   } /* while (1) */
}

