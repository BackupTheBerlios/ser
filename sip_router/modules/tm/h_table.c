/*
 * $Id: h_table.c,v 1.57 2002/03/02 04:51:55 bogdan Exp $
 */

#include "hash_func.h"
#include "h_table.h"
#include "../../dprint.h"
#include "sh_malloc.h"
#include "../../md5utils.h"

/* containes of a cell and the cell's body itself */
void free_cell( struct cell* dead_cell )
{
	int i;
	struct retrans_buff* rb;
	char *b;

	/* UA Server */
	release_cell_lock( dead_cell );
	shm_lock();
	if ( dead_cell->inbound_request )
		sip_msg_free_unsafe( dead_cell->inbound_request );
	if ((b=dead_cell->outbound_response.retr_buffer))
		shm_free_unsafe( b );

	/* UA Clients */
	for ( i =0 ; i<dead_cell->nr_of_outgoings;  i++ )
	{
		/* outbound requests*/
		if ( (rb=dead_cell->outbound_request[i]) )
		{
			if (rb->retr_buffer) shm_free_unsafe( rb->retr_buffer );
			dead_cell->outbound_request[i] = NULL;
			shm_free_unsafe( rb );
		}
		/* outbound ACKs, if any */
		if ( (rb=dead_cell->outbound_ack[i]) )
			shm_free_unsafe( rb );
		/* local cancel , if any */
		if ( (rb=dead_cell->outbound_cancel[i]) )
			shm_free_unsafe( rb );
		/* inbound response */
		if ( dead_cell -> inbound_response[i] )
			sip_msg_free_unsafe( dead_cell->inbound_response[i] );
	}
	/* mutex */
	/* release_cell_lock( dead_celle ); */
	/* the cell's body */
	shm_free_unsafe( dead_cell );
	shm_unlock();
}




/* Release all the data contained by the hash table. All the aux. structures
  *  as sems, lists, etc, are also released
  */
void free_hash_table( struct s_table *hash_table )
{
   struct cell* p_cell;
   struct cell* tmp_cell;
   int   i;

   if (hash_table)
   {
      /* remove the data contained by each entry */
      for( i = 0 ; i<TABLE_ENTRIES; i++)
      {
         release_entry_lock( (hash_table->entrys)+i );
         /* delete all synonyms at hash-collision-slot i */
         for( p_cell = hash_table->entrys[i].first_cell; p_cell; p_cell = tmp_cell )
         {
            tmp_cell = p_cell->next_cell;
            free_cell( p_cell );
         }
      }

      /* the mutexs for sync the lists are released*/
      for ( i=0 ; i<NR_OF_TIMER_LISTS ; i++ )
         release_timerlist_lock( &(hash_table->timers[i]) );

      sh_free( hash_table );
   }
}




/*
  */
struct s_table* init_hash_table()
{
   struct s_table*  hash_table;
   int       i;

   /*allocs the table*/
   hash_table = (struct s_table*)sh_malloc( sizeof( struct s_table ) );
   if ( !hash_table )
      goto error;

   memset( hash_table, 0, sizeof (struct s_table ) );

   /* try first allocating all the structures needed for syncing */
   if (lock_initialize()==-1)
     goto error;

   /* inits the entrys */
   for(  i=0 ; i<TABLE_ENTRIES; i++ )
   {
      init_entry_lock( hash_table , (hash_table->entrys)+i );
      hash_table->entrys[i].next_label = rand();
   }

   /* inits the timers*/
   for(  i=0 ; i<NR_OF_TIMER_LISTS ; i++ )
      init_timer_list( hash_table, i );

   return  hash_table;

error:
   free_hash_table( hash_table );
   lock_cleanup();
   return 0;
}



struct cell*  build_cell( struct sip_msg* p_msg )
{
	struct cell* new_cell;
#ifndef USE_SYNONIM
	str          src[5];
#endif

	/* do we have the source for the build process? */
	if (!p_msg)
		return NULL;

	/* allocs a new cell */
	new_cell = (struct cell*)sh_malloc( sizeof( struct cell ) );
	if  ( !new_cell )
		return NULL;

	/* filling with 0 */
	memset( new_cell, 0, sizeof( struct cell ) );

	new_cell->outbound_response.retr_timer.tg=TG_RT;
	new_cell->outbound_response.fr_timer.tg=TG_FR;
	new_cell->wait_tl.tg=TG_WT;
	new_cell->dele_tl.tg=TG_DEL;

	/* hash index of the entry */
	new_cell->hash_index = p_msg->hash_index;
	/* mutex */
	/* ref counter is 0 */
	/* all pointers from timers list tl are NULL */
	new_cell->wait_tl.payload = new_cell;
	new_cell->dele_tl.payload = new_cell;

	new_cell->inbound_request =  sip_msg_cloner(p_msg) ;
	if (!new_cell->inbound_request)
		goto error;
	new_cell->relaied_reply_branch   = -1;
	new_cell->T_canceled = T_UNDEFINED;
	new_cell->tag=&(get_to(new_cell->inbound_request)->tag_value);
#ifndef USE_SYNONIM
	src[0]= p_msg->from->body;
	src[1]= p_msg->to->body;
	src[2]= p_msg->callid->body;
	src[3]= p_msg->first_line.u.request.uri;
	src[4]= get_cseq( p_msg )->number;
	MDStringArray ( new_cell->md5, src, 5 );
#endif

	init_cell_lock(  new_cell );

	return new_cell;

error:
	sh_free(new_cell);
	return NULL;
}




/*  Takes an already created cell and links it into hash table on the
  *  appropiate entry.
  */
void    insert_into_hash_table_unsafe( struct s_table *hash_table,  struct cell * p_cell )
{
	struct entry* p_entry;

	/* locates the apropiate entry */
	p_entry = &hash_table->entrys[ p_cell->hash_index ];

	p_cell->label = p_entry->next_label++;
	if ( p_entry->last_cell )
	{
		p_entry->last_cell->next_cell = p_cell;
		p_cell->prev_cell = p_entry->last_cell;
	} else p_entry->first_cell = p_cell;

	p_entry->last_cell = p_cell;
}

void insert_into_hash_table( struct s_table *hash_table,  struct cell * p_cell )
{
	lock( &(hash_table->entrys[ p_cell->hash_index ].mutex) );
	insert_into_hash_table_unsafe( hash_table,  p_cell );
	unlock( &(hash_table->entrys[ p_cell->hash_index ].mutex) );
}




/*  Un-link a  cell from hash_table, but the cell itself is not released
  */
void remove_from_hash_table( struct s_table *hash_table,  struct cell * p_cell )
{
   struct entry*  p_entry  = &(hash_table->entrys[p_cell->hash_index]);

   /* unlink the cell from entry list */
   lock( &(p_entry->mutex) );

   if ( p_cell->prev_cell )
      p_cell->prev_cell->next_cell = p_cell->next_cell;
   else
      p_entry->first_cell = p_cell->next_cell;

   if ( p_cell->next_cell )
      p_cell->next_cell->prev_cell = p_cell->prev_cell;
   else
      p_entry->last_cell = p_cell->prev_cell;

   unlock( &(p_entry->mutex) );
}






