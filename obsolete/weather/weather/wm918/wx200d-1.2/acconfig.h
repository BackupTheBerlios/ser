/*
 * @(#)$Id: acconfig.h,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file is build-time configuration options for the WX200 programs
 *
 */

				/* @TOP@ */
				/* @BOTTOM@ */
/* default hostname for the clients to connect to ("localhost") */
#define HOST "localhost"

/* default port for the server to listen on and the clients to connect to */
#define PORT 9753

/* maximum number of simultaneous client connections allowed by the server */
#define CONNECTIONS 128

/*  place to look for WX200, if no WX200 environment var (e.g. /dev/ttyS1) */
#define WX200 "/dev/wx200"

/* where to write the daily wx200 data files */
#undef PACKAGE_DATA_DIR

/* default config for postgres support */
#define POSTGRES_CONN 	"dbname=postgres port=5432 user=postgres password=postgres"

/* default table name for postgres (the last characters could be the
   location of wx station) */
#define POSTGRES_TABLE  "wx_data"

/* default record interval (register data every POSTGRES_INTERVAL seconds) */
#define POSTGRES_INTERVAL 30
