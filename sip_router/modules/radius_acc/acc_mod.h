/*
 * Accounting module
 *
 * $Id: acc_mod.h,v 1.1 2002/08/08 17:28:55 ssi Exp $
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
