/*
 * @(#)$Id: wxread.c,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file reads the serial port directly and sends it to stdout.
 *
 */

#include <stdio.h>
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
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "wx200.h"
#include "serial.h"

char *progname;
char error[80];

/* main program */
int
main(int argc, char **argv)
{
  int c, fd;
  char *device = NULL;

  progname = strrchr(argv[0], '/');
  if (progname == NULL)
    progname = argv[0];
  else
    progname++;
  if (argc > 1)
    if (argv[1] != NULL)
      device = argv[1];
  if ((fd = init_serial(device)) < 0)
    exit(fd);
  while (read(fd, &c, 1) == 1) {
    putchar(c);
    fflush(stdout);
  }
  cleanup_serial(fd);
  exit(0);
}
