/*
 * @(#)$Id: wxfilter.c,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements a conversion filter (stdin to stdout).
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include "wx200.h"

int
main(int argc, char **argv) {
  const char *flags = "Hhz";
  unsigned int opt, bufnum, opt_z = 0;

  while ((opt = getopt(argc, argv, flags)) != EOF) {
    switch (opt) {
    case '?':
    case 'h':
    case 'H':
      printf("usage: wxz [-h | -z]
	-h	show this help and exit
	-z	produce abbreviated binary, instead of tab-delimited ASCII\n");
      exit(0);
    case 'z':
      opt_z = 1;
    }
  }

  wx200bufinit();
  while ((bufnum = wx200bufread(0)) != EOF) {
    switch (bufnum) {
    case GROUP8:
    case GROUP9:
    case GROUPA:
    case GROUPB:
    case GROUPC:
    case GROUPF:
      if (opt_z)		/* compress */
	write(1, groupbuf[bufnum], grouplen[bufnum]);
      else if (wx200tab(bufnum)) /* tab-delimit */
	write(1, tabbuf, strlen(tabbuf));
    }
  }
  exit(0);
}
