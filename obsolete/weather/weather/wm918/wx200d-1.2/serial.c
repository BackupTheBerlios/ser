/*
 * @(#)$Id: serial.c,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the POSIX termios serial interface to WX200.
 * termio to termios conversion by Pat Jensen <patj@futureunix.net>
 *
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "wx200.h"

int wx200stationtype = 0;	/* 0=WX200/WM918, 1=WMR918 */

int speed=B9600, bits=CS8, stopbits=0, parity=0;

extern char *progname, error[];
char *device = NULL;		/* serial device name */
struct termios stbuf, svbuf;	/* termios: svbuf=saved, stbuf=set */

/* serial cleanup routine called as the program exits */
void
cleanup_serial(int fd)
{
  if (fd > 0) {
    if (tcsetattr(fd, TCSANOW, &svbuf) < 0) {
      sprintf(error, "%s: can't tcsetattr device %s", progname, device);
      perror(error);
    }
    close(fd);
  }
}

/* return file descriptor of successfully opened serial device or negative # */
int
init_serial(char *dev)
{
  int fd = -1;			/* serial device file descriptor */

  if (dev != NULL)
    device = dev;
  else if ((device = getenv("WX200")) == NULL) /* default to env var */
    device = WX200;		/* or built-in default */
  if (strstr(device, "wmr"))	/* e.g. /dev/wmr918, /dev/wmr968 */
    wx200stationtype = 1;
  if ((fd = open(device, O_RDONLY)) < 0) {
    sprintf(error, "%s: can't open device %s", progname, device);
    perror(error);
    return(-1);
  }

  if (tcgetattr(fd, &svbuf) < 0) {
    sprintf(error, "%s: can't tcgetattr device %s", progname, device);
    perror(error);
    close(fd);
    return(-2);
  }

  memcpy(&stbuf, &svbuf, sizeof(struct termios));
  stbuf.c_iflag = 0;
  stbuf.c_oflag = 0;
  stbuf.c_lflag = 0;
  stbuf.c_cflag = speed | bits | stopbits | parity | CLOCAL | CREAD;

  if (tcsetattr(fd, TCSANOW, &stbuf) < 0) {
    sprintf(error, "%s: can't tcsetattr device %s", progname, device);
    perror(error);
    close(fd);
    return(-3);
  }
  return(fd);
}
