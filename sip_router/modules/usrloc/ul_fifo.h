/*
 *
 * $Id: ul_fifo.h,v 1.4 2002/08/27 13:31:25 janakj Exp $
 *
 *
 */

#ifndef _UL_FIFO_H
#define _UL_FIFO_H

/* FIFO commands */
#define UL_STATS	"ul_stats"
#define UL_RM		"ul_rm"
#define UL_DUMP         "ul_dump"
#define UL_FLUSH        "ul_flush"

/* buffer dimensions */
#define MAX_TABLE 128
#define MAX_USER 256

int init_ul_fifo(void);

#endif
