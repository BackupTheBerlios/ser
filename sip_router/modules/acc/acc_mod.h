/*
 * Accounting module
 *
 * $Id: acc_mod.h,v 1.1 2002/05/10 01:04:45 jku Exp $
 */

#ifndef _ACC_H
#define _ACC_H

/* module parameter declaration */
extern int use_db;
extern char *db_url;
extern char *uid_column;
extern char *db_table;
extern int log_level;
extern int early_media;
extern int failed_transactions;
extern int flagged_only;

#endif
