/*
 * @(#)$Id: wxdebug.c,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements a debuging filter (stdin to stdout).
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include "wx200.h"

#define ALARM(flag)	(flag ? 'A' : 'a')
#define OR(flag)	(flag ? 'O' : 'o')
#define ERR(flag)	(flag ? 'E' : 'e')

void
showhex(unsigned char *buf, int num)
{
  int i;
  for (i = 0; i < num; i++) {
    printf("%s%02x", (i % 2 ? " " : ""), buf[i]);
  }
  printf("\n");
}

int
main(int argc, char **argv)
{
  const char *flags = "Hh0189abcfupx";
  int opt, group, eight, nine, a, b, c, f, u, p = 1, x = -1;
  WX_UNK prev;

  eight = nine = a = b = c = f = u = 1;
  while ((opt = getopt(argc, argv, flags)) != EOF) {
    switch (opt) {
    case '?':
    case 'h':
    case 'H':
      printf("usage: debug [-0189abcfupx]\n"
	     "\t0\tall groups off\n\t1\tall groups on (default)\n"
	     "\tpx\ttoggle plain mode (default on) and hex mode (default off)\n"
	     "\t89abcfu\ttoggle group on/off, f=timestamp, u=unknowns\n");
      exit(0);
    case '0':
      eight = nine = a = b = c = f = u = -1;
      break;
    case '1':
      eight = nine = a = b = c = f = u = 1;
      break;
    case '8':
      eight *= -1;
      break;
    case '9':
      nine *= -1;
      break;
    case 'a':
      a *= -1;
      break;
    case 'b':
      b *= -1;
      break;
    case 'c':
      c *= -1;
      break;
    case 'f':
      f *= -1;
      break;
    case 'u':
      u *= -1;
      break;
    case 'p':
      p *= -1;
      break;
    case 'x':
      x *= -1;
      break;
    }
  }
  wx200bufinit();
  while ((group = wx200bufread(0)) != EOF) {
    if (group >= 0)
      wx200parse(group);
    switch (group) {
    case GROUPF:
      if (f < 0) break;
      if (x > 0) showhex(groupbuf[group], grouplen[group]);
      if (p > 0)
	printf("f %02d:%02d:%02d\n",
	       wx.td.clock.hour, wx.td.clock.min, wx.td.clock.sec);
      break;
    case GROUP8:
      if (eight < 0) break;
      if (x > 0) showhex(groupbuf[group], grouplen[group]);
      if (p > 0)
	printf("8 %02d/%02d:%02d:%02d:%02d %c%02d:%02d"
	       " %c%d-%d=%d%c%02d/%02d:%02d:%02d<%c%d%c<%d%c%02d/%02d:%02d:%02d"
	       " %c%d-%d=%d%c%02d/%02d:%02d:%02d<%c%d%c<%d%c%02d/%02d:%02d:%02d"
	       " f%d\n",

	       wx.td.clock.mon, wx.td.clock.day,
	       wx.td.clock.hour, wx.td.clock.min, wx.td.clock.sec,
	       ALARM(wx.td.a.on), wx.td.a.hi, wx.td.a.lo,

	       ALARM(wx.humid.ina.on), wx.humid.ina.lo, wx.humid.ina.hi,
	       wx.humid.inlo.val, OR(wx.humid.inlo.or),
	       wx.humid.inlo.mon, wx.humid.inlo.day,
	       wx.humid.inlo.hour, wx.humid.inlo.min,
	       ERR(wx.humid.in.err), wx.humid.in.val, OR(wx.humid.in.or),
	       wx.humid.inhi.val, OR(wx.humid.inhi.or),
	       wx.humid.inhi.mon, wx.humid.inhi.day,
	       wx.humid.inhi.hour, wx.humid.inhi.min,

	       ALARM(wx.humid.outa.on), wx.humid.outa.lo, wx.humid.outa.hi,
	       wx.humid.outlo.val, OR(wx.humid.outlo.or),
	       wx.humid.outlo.mon, wx.humid.outlo.day,
	       wx.humid.outlo.hour, wx.humid.outlo.min,
	       ERR(wx.humid.out.err), wx.humid.out.val, OR(wx.humid.out.or),
	       wx.humid.outhi.val, OR(wx.humid.outhi.or),
	       wx.humid.outhi.mon, wx.humid.outhi.day,
	       wx.humid.outhi.hour, wx.humid.outhi.min,

	       wx.td.format);
      break;
    case GROUP9:
      if (nine < 0) break;
      if (x > 0) showhex(groupbuf[group], grouplen[group]);
      if (p > 0)
	printf("9"
	       " %c%d-%d=%d%c%02d/%02d:%02d:%02d<%c%d%c<%d%c%02d/%02d:%02d:%02d"
	       " %c%d-%d=%d%c%02d/%02d:%02d:%02d<%c%d%c<%d%c%02d/%02d:%02d:%02d"
	       " f%d\n",


	       ALARM(wx.temp.ina.on), wx.temp.ina.lo, wx.temp.ina.hi,
	       wx.temp.inlo.val, OR(wx.temp.inlo.or),
	       wx.temp.inlo.mon, wx.temp.inlo.day,
	       wx.temp.inlo.hour, wx.temp.inlo.min,
	       ERR(wx.temp.in.err), wx.temp.in.val, OR(wx.temp.in.or),
	       wx.temp.inhi.val, OR(wx.temp.inhi.or),
	       wx.temp.inhi.mon, wx.temp.inhi.day,
	       wx.temp.inhi.hour, wx.temp.inhi.min,

	       ALARM(wx.temp.outa.on), wx.temp.outa.lo, wx.temp.outa.hi,
	       wx.temp.outlo.val, OR(wx.temp.outlo.or),
	       wx.temp.outlo.mon, wx.temp.outlo.day,
	       wx.temp.outlo.hour, wx.temp.outlo.min,
	       ERR(wx.temp.out.err), wx.temp.out.val, OR(wx.temp.out.or),
	       wx.temp.outhi.val, OR(wx.temp.outhi.or),
	       wx.temp.outhi.mon, wx.temp.outhi.day,
	       wx.temp.outhi.hour, wx.temp.outhi.min,

	       wx.temp.format);
      break;
    case GROUPA:
      if (a < 0) break;
      if (x > 0) showhex(groupbuf[group], grouplen[group]);
      if (p > 0)
	printf("a"
	       " %c%d=%d%c%02d/%02d:%02d:%02d<%c%d%c<%d%c%02d/%02d:%02d:%02d"
	       " %c%d=%d%c%02d/%02d:%02d:%02d<%c%d%c<%d%c%02d/%02d:%02d:%02d"
	       " l%c%d%c s%c%d%c p%dt%df%d\n",

	       ALARM(wx.dew.ina.on), wx.dew.ina.lo,
	       wx.dew.inlo.val, OR(wx.dew.inlo.or),
	       wx.dew.inlo.mon, wx.dew.inlo.day,
	       wx.dew.inlo.hour, wx.dew.inlo.min,
	       ERR(wx.dew.in.err), wx.dew.in.val, OR(wx.dew.in.or),
	       wx.dew.inhi.val, OR(wx.dew.inhi.or),
	       wx.dew.inhi.mon, wx.dew.inhi.day,
	       wx.dew.inhi.hour, wx.dew.inhi.min,

	       ALARM(wx.dew.outa.on), wx.dew.outa.lo,
	       wx.dew.outlo.val, OR(wx.dew.outlo.or),
	       wx.dew.outlo.mon, wx.dew.outlo.day,
	       wx.dew.outlo.hour, wx.dew.outlo.min,
	       ERR(wx.dew.out.err), wx.dew.out.val, OR(wx.dew.out.or),
	       wx.dew.outhi.val, OR(wx.dew.outhi.or),
	       wx.dew.outhi.mon, wx.dew.outhi.day,
	       wx.dew.outhi.hour, wx.dew.outhi.min,

	       ERR(wx.baro.local.err), wx.baro.local.val, OR(wx.baro.local.or),
	       ERR(wx.baro.sea.err), wx.baro.sea.val, OR(wx.baro.sea.or),

	       wx.baro.pred, wx.baro.trend, wx.baro.format);
      break;
    case GROUPB:
      if (b < 0) break;
      if (x > 0) showhex(groupbuf[group], grouplen[group]);
      if (p > 0)
	printf("b %c%d r%c%d%c y%d%c t%d%c %02d/%02d:%02d:%02d f%d\n",
	       ALARM(wx.rain.a.on), wx.rain.a.hi,
	       ERR(wx.rain.rate.err), wx.rain.rate.val, OR(wx.rain.rate.or),
	       wx.rain.yest.val, OR(wx.rain.yest.or),
	       wx.rain.total.val, OR(wx.rain.total.or),
	       wx.rain.total.mon, wx.rain.total.day,
	       wx.rain.total.hour, wx.rain.total.min, wx.rain.format);
      break;
    case GROUPC:
      if (c < 0 && u < 0) break;
      if (c > 0 && x > 0) showhex(groupbuf[group], grouplen[group]);
      if (c > 0 && p > 0)
	printf("c %c%d"
	       " g%c%03d@%c%d%c~%c%03d@%c%d%c<%03d@%d%c=%02d/%02d:%02d:%02d"
	       " %c%d %d%c%02d/%02d:%02d:%02d<%c%d%c"
	       " p%d b%d s%d/%d/%d f%d\n",

	       ALARM(wx.wind.a.on), wx.wind.a.hi,

	       ERR(wx.wind.gdir.err), wx.wind.gdir.val,
	       ERR(wx.wind.gspeed.err), wx.wind.gspeed.val,
	       OR(wx.wind.gspeed.or),
	       ERR(wx.wind.adir.err), wx.wind.adir.val,
	       ERR(wx.wind.aspeed.err), wx.wind.aspeed.val,
	       OR(wx.wind.aspeed.or),
	       wx.wind.dirhi.val, wx.wind.speedhi.val,
	       OR(wx.wind.speedhi.or),
	       wx.wind.speedhi.mon, wx.wind.speedhi.day,
	       wx.wind.speedhi.hour, wx.wind.speedhi.min,

	       ALARM(wx.chill.a.on), wx.chill.a.lo,
	       wx.chill.low.val, OR(wx.chill.low.or),
	       wx.chill.low.mon, wx.chill.low.day,
	       wx.chill.low.hour, wx.chill.low.min,
	       ERR(wx.chill.temp.err), wx.chill.temp.val, OR(wx.chill.temp.or),

	       wx.gen.power, wx.gen.lowbat,
	       wx.gen.section, wx.gen.screen, wx.gen.subscreen, wx.wind.format);
      if (u > 0 && p > 0) {
	if (memcmp(&wx.unk, &prev, sizeof(wx.unk))) {
	  printf("u %02d/%02d:%02d:%02d:%02d "
		 "5=%x,32=%x,33=%x/13=%x,15=%x,28=%x,30=%x,31=%x"
		 "/5=%x,6=%x,28=%x,29=%x/2=%x,10=%x,12=%x"
		 "/14=%x,15=%x,21=%x,23=%x,24=%x,25=%x\n",
		 wx.td.clock.mon, wx.td.clock.day,
		 wx.td.clock.hour, wx.td.clock.min, wx.td.clock.sec,
		 wx.unk.eightf5, wx.unk.eightf32, wx.unk.eightf33,
		 wx.unk.ninef13, wx.unk.ninef15, wx.unk.ninef28,
		 wx.unk.ninef30, wx.unk.ninef31, wx.unk.af5, wx.unk.af6,
		 wx.unk.af28, wx.unk.af29, wx.unk.bf2, wx.unk.bf10,
		 wx.unk.bf12, wx.unk.cf14, wx.unk.cf15, wx.unk.cf21,
		 wx.unk.cf23, wx.unk.cf24,wx.unk.cf25);
	  memcpy(&prev, &wx.unk, sizeof(wx.unk));
	}
      }
      break;
    }
  }
  exit(0);
}
