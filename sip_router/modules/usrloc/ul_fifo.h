/*
 *
 * $Id: ul_fifo.h,v 1.6 2002/09/03 23:28:58 janakj Exp $
 *
 *
 */

#ifndef _UL_FIFO_H
#define _UL_FIFO_H

/* FIFO commands */
#define UL_STATS	"ul_stats"
#define UL_RM		"ul_rm"
#define UL_RM_CONTACT   "ul_rm_contact"
#define UL_DUMP         "ul_dump"
#define UL_FLUSH        "ul_flush"
#define UL_ADD          "ul_add"

/* buffer dimensions */
#define MAX_TABLE 128
#define MAX_USER 256

int init_ul_fifo(void);

#endif
