/*
 * @(#)$Id: buffer.c,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file moves bytes from a file descriptor into global group buffers.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "wx200.h"

unsigned char groupbuf[GROUPS * 2][GROUP_LENMAX];
int grouplen[] = GROUP_LENGTHS;

#ifdef BUFDEBUG
#undef BUFDEBUG
#define BUFDEBUG(format, arg) fprintf(stderr, format, arg)
#else
#define BUFDEBUG(format, arg) ;
#endif

/* call this initializer before using groupbuf or to reset wx200bufread */
void
wx200bufinit()
{
  int i;			/* insert group numbers as first byte */

  for (i = 0; i < GROUPS ; i++) {
    memset(groupbuf[i + GROUPS], 0, GROUP_LENMAX);
    groupbuf[i][0] = groupbuf[i + GROUPS][0] = 0x0f | (i << 4);
  }
}

/* (internal) read num bytes from fd into buf, returning 1 if checksums match */
int
wx200getsome(int fd, unsigned char *buf, int num)
{
  int i, sum;
  unsigned char c;

  i = 1;
  sum = buf[0];
  BUFDEBUG("%02x:", sum);
  while (i <= num) {
    if (read(fd, &c, 1) != 1)
      return(0);
    else {
      BUFDEBUG(" %02x", c);
      if (i < num)
	sum += c;
      buf[i++] = c;
    }
  }
  sum &= 0xff;
  BUFDEBUG(" %c", (c == sum ? '=' : '!'));
  BUFDEBUG("%02x\n", sum);
  return(c == sum ? 1 : 0);
}

/* read from given fd and return the changed groupbuf's index or negative */
int
wx200bufread(int fd)
{
  unsigned char chr;
  int i, j, group, ret;

  if ((ret = read(fd, &chr, 1)) < 0)
    return(ret);		/* some read error */
  if (ret == 0)
    return(EOF);		/* end of file */
  ret = -2;
  switch (chr) {
/*   case 0x0f: */
/*   case 0x1f: */
/*   case 0x2f: */
/*   case 0x3f: */
/*   case 0x4f: */
/*   case 0x5f: */
/*   case 0x6f: */
/*   case 0x7f: */
  case 0x8f:			/* 1000 */
  case 0x9f:			/* 1001 */
  case 0xaf:			/* 1010 */
  case 0xbf:			/* 1011 */
  case 0xcf:			/* 1100 */
  case 0xdf:			/* 1101 */
  case 0xef:			/* 1110 */
  case 0xff:			/* 1111 */
    group = (chr & 0xf0) >> 4;
    if (wx200getsome(fd, groupbuf[group], grouplen[group] - 1) == 1) {
      if (group == GROUP8) {
	memcpy(&groupbuf[GROUPF][1], &groupbuf[GROUP8][1], 3);
	j = 0;			/* 8F timestamp -> FF, then calc checksum */
	for (i = 0; i < 4; i++)
	  j += groupbuf[GROUPF][i];
	groupbuf[GROUPF][4] = j & 0xff;
	if (!memcmp(&groupbuf[GROUP8][4], &groupbuf[GROUP8 + GROUPS][4], 30))
	  group = GROUPF;
      }
      if (memcmp(groupbuf[group], groupbuf[group + GROUPS], grouplen[group]))
	ret = group;
      memcpy(groupbuf[group + GROUPS], groupbuf[group], grouplen[group]);
    }
    break;
  }
  return(ret);
}
