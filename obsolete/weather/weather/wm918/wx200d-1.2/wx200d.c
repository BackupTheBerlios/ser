/*
 * @(#)$Id: wx200d.c,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2002 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the WX200 / WM918 data logger and server daemon.
 *
 */

#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <signal.h>
#include <sys/param.h>
#include <utime.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include "wx200.h"
#include "serial.h"
#define POSTGRES	(defined(HAVE_LIBPQ) && defined(HAVE_LIBPQ_FE_H))
#if POSTGRES
#include "pg_api.h"
#endif

#ifdef MINDEBUG			/* write files every minute instead of day? */
#define MON(time)	(0)
#define DAY(time)	(time->tm_min)
#else
#define MON(time)	(time->tm_mon + 1)
#define DAY(time)	(time->tm_mday)
#endif

char error[80];
char *progname;
int *current = 0;		/* pointer to current socket handle */

#if POSTGRES
PGconn *pConn = NULL;		/* pointer to current postgres connection */
#endif

void
usage()
{
  printf("usage: %s [-h] [-d] [-a] [-b] [-z] [-p port] [-r] [-s device] [-w path]\n",
	 progname );
#if POSTGRES
  printf("\t[-g] [-c connection] [-t tablename] [-i interval]\n" );
#endif
  printf("  -h	show this help and exit
  -d	debug; don't fork to be a daemon
  -a	don't record tab-delimited ASCII data files
  -b	don't record abbreviated Binary data files
  -z	don't gZip daily data files after collection
  -p port	listen for client connections on port instead of %d
  -r		read a wireless WMR918 station instead of a wired WX200/WM918
  -s device	get WX200 data from device instead of %s
  -w path	write files in path instead of %s
", PORT, WX200, PACKAGE_DATA_DIR);
#if POSTGRES
  printf(
"  -g		enables recording values into a postgres database
  -c connection	postgres database connection instead of \n\t%s
  -t tablename	postgres table name instead of %s
  -i interval	postgres record interval instead of %d secs
", POSTGRES_CONN, POSTGRES_TABLE, POSTGRES_INTERVAL);
#endif

  exit(0);
}

int
stationbufread(int fd)
{
  if (wx200stationtype == 0)	/* original WX200/WM918 */
    return wx200bufread(fd);
  else if (wx200stationtype == 1) /* WMR918 by Dominique Le Foll */
    return wmr918bufread(fd);
  return -1;			/* huh? */
}

void
pipe_handler(int sig)		/* SIGPIPE signal handler */
{
  signal(SIGPIPE, SIG_IGN);
  if (sig == SIGPIPE) {		/* client went away */
    shutdown(*current, 2);
    close(*current);
    *current = -1;
  }
}

void
term_handler(int sig)		/* SIGTERM signal handler */
{
#if POSTGRES
  if (pConn)
    pg_finish( pConn );
#endif
  exit( 0 );
}

int
main(int argc, char **argv)
{
  int sd, pid, s, clen = sizeof(struct sockaddr_in), fd[CONNECTIONS];
  int *max = 0, afile = -1, bfile = -1, /* mfile = -1, */ c, first = 1;
  int pday = -1, pmon = -1, pwday = -1;
  int opt_a = 0, opt_b = 0, opt_d = 0, opt_z = 0, opt_p = PORT;
  int bufnum, i, j, bit, bits;
  time_t nowtime;
  struct tm *now;
  char *opt_s = NULL, *opt_w = PACKAGE_DATA_DIR;
  char afilename[128], bfilename[128], /* mfilename[128], */ command[256];
#if POSTGRES
  int opt_g = 0;
  char *opt_c = POSTGRES_CONN;
  char *opt_t = POSTGRES_TABLE;
  char *opt_i = NULL;
#endif
  struct sockaddr_in server, client;
  struct in_addr bind_address;
  fd_set rfds;
  struct timeval tv;
  struct utimbuf ut;
#if POSTGRES
  int nPgRet;			/* return of postgres insertion */
  time_t lasttime;		/* last time we made an insert */
  time_t pg_interval;		/* write to db every "pg_interval" seconds */
#endif

  wx200ignoreclock = 1;		/* computer clock overrides WX200's */
  progname = strrchr(argv[0], '/');
  if (progname == NULL)
    progname = argv[0];		/* global for error messages, usage */
  else
    progname++;
  while ((c = getopt(argc, argv, "Hhdrabzp:s:w:gc:t:i:")) != EOF) {
    switch(c) {
    case '?':
    case 'h':
    case 'H':			/* help */
      usage();
      break;
    case 'd':			/* do not fork and become a daemon */
      opt_d = 1;
      break;
    case 'a':			/* no tab-delimited ASCII log */
      opt_a = 1;
      break;
    case 'b':			/* no binary data log */
      opt_b = 1;
      break;
    case 'z':			/* do not gzip nightly after collection */
      opt_z = 1;
      break;
    case 'p':			/* port to use */
      opt_p = strtol(optarg, NULL, 0);
      break;
    case 's':			/* serial device to use */
      opt_s = optarg;
      break;
    case 'w':			/* path to write to */
      opt_w = optarg;
      break;
    case 'r':			/* select a WMR wireless radio station */
      wx200stationtype = 1;
      break;
#if POSTGRES
    case 'g':			/* enable postgres recording */
      opt_g = 1;
      break;
    case 'c':			/* postgres connection string */
      opt_c = optarg;
      break;
    case 't':			/* postgres tablename */
      opt_t = optarg;
      break;
    case 'i':			/* postgres recording time interval */
      opt_i = optarg;
      break;
#endif
    }
  }

  server.sin_family = AF_INET;
  bind_address.s_addr = htonl(INADDR_ANY);
  server.sin_addr = bind_address;
  server.sin_port = htons(opt_p);

  if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
    sprintf(error, "%s: socket", progname);
    perror(error);
    cleanup_serial(sd);
    exit(10);
  }
  {
    /* <dirkx@covalent.net> / April 2001 Minor change to allow quick
     * (re)start of deamon or client while there are pending
     * conncections during the quit. To avoid addresss/port in use
     * error.  */
    int i = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) == -1) {
      sprintf(error, "%s: setsockopt", progname);
      perror(error);
    }
  }
  if (bind(s, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) == -1) {
    sprintf(error, "%s: bind", progname);
    perror(error);
    cleanup_serial(sd);
    exit(11);
  }
  if (listen(s, CONNECTIONS) == -1) {
    sprintf(error, "%s: listen", progname);
    perror(error);
    cleanup_serial(sd);
    exit(12);
  }

  /* If another server is running, the above will fail because of the
     busy port.  So no locking is needed here! */
  if ((sd = init_serial(opt_s)) < 0)
    exit(sd);

  umask(0022);
  wx200bufinit();		/* initialize all variables */
  wx200tabinit();
  for (i = 0; i < CONNECTIONS; i++) fd[i] = -1;
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  openlog(progname, LOG_PID | (opt_d ? LOG_PERROR : 0), LOG_LOCAL4);

  if (!opt_d) {			/* setup has worked; now become a daemon? */
    if ((pid = fork()) == -1) {
      syslog(LOG_ERR, "can't fork() to become daemon: %m");
      cleanup_serial(sd);
      exit(20);
    } else if (pid)
      exit (0);
    setsid();
    for (i = 0; i < NOFILE; i++)
      if (i != sd && i != s) close(i);
  }

  /* catch signals to close the database connection */
  signal( SIGTERM, term_handler );/* termination */
  signal( SIGPWR, term_handler ); /* power failure */

#if POSTGRES
  if( opt_g ) {			/* postgres recording enabled */
    if( opt_i != NULL )
      pg_interval = atol( opt_i );
    else
      pg_interval = POSTGRES_INTERVAL;
    lasttime = 0;
    pConn = pg_init( opt_c, opt_t );
  }
#endif

  while (1) {			/* live forever, listening and logging */
    FD_ZERO(&rfds);
    FD_SET(s, &rfds);
    if (select(s + 1, &rfds, NULL, NULL, &tv) > 0) {
      for (current = fd;
	   (*current > 0) && (current < fd + CONNECTIONS - 1); current++);
      if (current > max) max = current;
      if ((*current = accept(s, (struct sockaddr *)&client, &clen)) != -1)
	for (i = 0; i < GROUPS; i++) { /* give current data to new client */
	  write(*current, groupbuf[i], grouplen[i]);
	}
    }
    FD_ZERO(&rfds);
    FD_SET(sd, &rfds);
    while ((select(sd + 1, &rfds, NULL, NULL, &tv) > 0)
	   && ((bufnum = stationbufread(sd)) >= 0)) {
      switch (bufnum) {
      case GROUPF:		/* timestamp? check for new day */
      case GROUP1:
      case GROUP2:
      case GROUP3:
      case GROUP4:		/* 1-7 is a WMR918 multi-group (3 bits) */
      case GROUP5:
      case GROUP6:
      case GROUP7:
      case GROUP8:
	nowtime = time(NULL);
	now = localtime(&nowtime);
	if ((DAY(now) != pday) || first) {
	  if (!first) {		/* non-first files? close current ones */
	    if (bfile > -1) close(bfile);
	    if (afile > -1) close(afile);
	    ut.actime = ut.modtime = time(NULL); /* make sure .tab is as new */
	    utime(afilename, &ut);
	    if (!opt_z) {	/* compress yesterday's files */
	      if (bfile > -1 || afile > -1) {
		sprintf(command, "gzip %s %s &", bfile > -1 ? bfilename : "",
			afile > -1 ? afilename : "");
		system(command);
	      }
	    }
	  }
	  sprintf(bfilename, "%s/%02d%02d%02d.bin", opt_w,
		  1900 + now->tm_year, MON(now), DAY(now));
	  sprintf(afilename, "%s/%02d%02d%02d.tab", opt_w,
		  1900 + now->tm_year, MON(now), DAY(now));
	  /* 	  sprintf(mfilename, "%s/%02d%02d%02d.mem", opt_w, */
	  /* 		  1900 + now->tm_year, MON(now), DAY(now)); */
	  if (!opt_b &&
	      (bfile = open(bfilename, O_WRONLY|O_CREAT|O_APPEND, 00644)) < 0) {
	    syslog(LOG_ERR, "can't write %s: %m", bfilename);
	    bfile = -1;
	  }
	  if (!opt_a &&
	      (afile = open(afilename, O_WRONLY|O_CREAT|O_APPEND, 00644)) < 0) {
	    syslog(LOG_ERR, "can't write %s: %m", bfilename);
	    afile = -1;
	  }
	  if (!first) {		/* non-first file?  begin w/ current data */
	    if (bfile > -1)
	      for (i = 0; i < GROUPS; i++) {
		write(bfile, groupbuf[i], grouplen[i]);
	      }
	  }
	  wx200bufinit();	/* forget memory to re-get all groups */
	  wx200tabinit();
	  wx200parse(-1);	/* reset appropriate virtual memories to now */
	  pday = DAY(now);
	  pmon = MON(now);
	  pwday = now->tm_wday;
	  first = 0;
	}
      case GROUP9:
      case GROUPA:
      case GROUPB:
      case GROUPC:
	if (bufnum < GROUP8) {	/* WMR918 tweaks up to 3 groups at once */
	  bits = bufnum;
	  bufnum = GROUP8;
	  j = GROUPA;
	} else {		/* otherwise, just log this one group */
	  bits = 0x01;
	  j = bufnum;
	}
	for (i = bit = 0; bufnum <= j; bufnum++) {
	  if (bits & (0x01 << bit)) { /* this group changed? */
	    if (wx200tab(bufnum)) /* calls wx200parse(bufnum) */
	      i = 1;
	    if (bfile > -1)
	      write(bfile, groupbuf[bufnum], grouplen[bufnum]);
	    for (current = fd; current <= max; current++)
	      if (*current > 0) {	/* active socket connection */
		signal(SIGPIPE, pipe_handler);
		write(*current, groupbuf[bufnum], grouplen[bufnum]);
	      }
	  }
	  bit++;
	}
	if (afile > -1 && i)
	  write(afile, tabbuf, strlen(tabbuf));
#if POSTGRES
	if( opt_g ){		/* postgres recording enabled */
	  if( nowtime - lasttime > pg_interval ) {
	    lasttime = nowtime;
	    nPgRet = pg_insert( pConn, opt_t, &wx, 1 );
	    if( nPgRet == -2 ) { /* retrying the connection */
	      pConn = pg_init( opt_c, opt_t );
	    }
	  }
	}
#endif
      }
    }
    sleep(1);			/* no hurry, be a nice daemon */
  }
  cleanup_serial(sd);
  exit(0);
}
