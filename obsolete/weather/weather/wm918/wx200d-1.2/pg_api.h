/*
 * @(#)$Id: pg_api.h,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 2001 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file exports the Postgres database logging interface
 *
 * by Raul Luna <rlunaro@bigfoot.com>, 2001/05/12
 *
 */

#include <unistd.h>
#include "libpq-fe.h"

/* #include "wx200.h" */

#ifndef PG_API_H	/* sentinel */
#define PG_API_H

/* my own name */
#define PG_API_NAME "Postgres Interface"

/* some constants */
#define PG_MAX_TABLE_NAME 	256
#define PG_MAX_COMMAND		8192
#define PG_MAX_ERROR_MSG	PG_MAX_COMMAND

/* connect to the database: return TRUE if connected,
   false if not connected */
extern PGconn *pg_init( const char *szConnection, const char *szTableName );

/* insert data into the database */
extern int pg_insert( PGconn *pConn, const char *szTableName, WX *wx, int use_localtime );

/* disconnect from the database */
extern void pg_finish( PGconn *pConn);

#endif   /* sentinel */
