/*
 * $Id: timer.h,v 1.3 2001/12/10 16:11:42 andrei Exp $
 *
 *
 * timer related functions
 */


#ifndef timer_h
#define timer_h

typedef void (timer_function)(unsigned int ticks, void* param);


struct sr_timer{
	int id;
	timer_function* timer_f;
	void* t_param;
	unsigned int interval;
	
	unsigned int expires;
	
	struct sr_timer* next;
};



extern struct sr_timer* timer_list;



int init_timer();
/*register a periodic timer;
 * ret: <0 on errror*/
int register_timer(timer_function f, void* param, unsigned int interval);
unsigned int get_ticks();
void timer_ticker();

#endif
