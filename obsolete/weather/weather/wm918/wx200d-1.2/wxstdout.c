/*
 * @(#)$Id: wxstdout.c,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements a client which sends a data stream to stdout.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "wx200.h"

char *progname;

void
usage()
{
  printf("usage: %s [-h] [[host][:port]]
	-h	show this help and exit
", progname);
  exit(0);
}

int
main(int argc, char **argv)
{
  int c, socket;
  char *hostport = NULL;

  progname = strrchr(argv[0], '/');
  if (progname == NULL)
    progname = argv[0];
  else
    progname++;
  while ((c = getopt(argc, argv, "Hh")) != EOF) {
    switch(c) {
    case '?':
    case 'h':
    case 'H':			/* help */
      usage();
      break;
    }
  }
  if (optind < argc && argv[optind] != NULL)
    hostport = argv[optind];
  if ((socket = wx200open(hostport)) == -1)
    exit(1);
  while (read(socket, &c, 1) == 1) {
    write(1, &c, 1);
  }
  exit(wx200close(socket));
}
