/*
 * @(#)$Id: parse.c,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file parses the data from groupbuf into memory structures.
 *
 */

#include <unistd.h>
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
#include "wx200.h"
#include "parse.h"

WX wx;
int wx200ignoreclock = 0;

/* parse groupbuf[bufnum] into memory structures, returning # bytes parsed */
int
wx200parse(int bufnum)
{
  int ret;
  unsigned char *buf;
  time_t nowtime;
  struct tm *now = NULL;

  ret = 0;
  buf = groupbuf[bufnum];
  switch (bufnum) {
  case GROUPF:			/* Time */
  case GROUP8:			/* Time, Humidity */
    if (wx200ignoreclock) {
      nowtime = time(NULL);
      now = localtime(&nowtime);
      wx.td.clock.sec = now->tm_sec;
      wx.td.clock.min = now->tm_min;
      wx.td.clock.hour = now->tm_hour;
    } else {
      wx.td.clock.sec = NUM(buf[1]);
      wx.td.clock.min = NUM(buf[2]);
      wx.td.clock.hour = NUM(buf[3]);
    }
    ret = 3;
    if (bufnum == GROUPF)
      break;
    wx.td.clock.day = wx200ignoreclock ? now->tm_mday : NUM(buf[4]);
    wx.td.format = (buf[5] & 0x30) >> 4;
    wx.td.clock.mon = wx200ignoreclock ? now->tm_mon + 1 : LO(buf[5]);
    wx.unk.eightf5 = buf[5] & 0xc0;
    wx.td.a.nlo = wx.td.a.lo = NUM(buf[6]);
    wx.td.a.nhi = wx.td.a.hi = NUM(buf[7]);
    ERROR(wx.humid.in, buf[8], NUM(buf[8]));
    wx.humid.in.n = wx.humid.in.val;
    wx.humid.inhi.n = wx.humid.inhi.val = NUM(buf[9]);
    wx.humid.inhi.min = NUM(buf[10]);
    wx.humid.inhi.hour = NUM(buf[11]);
    wx.humid.inhi.day = NUM(buf[12]);
    wx.humid.inhi.mon = LO(buf[13]);
    wx.humid.inlo.n = wx.humid.inlo.val
      = 10 * LO(buf[14]) + HI(buf[13]);
    wx.humid.inlo.min = 10 * LO(buf[15]) + HI(buf[14]);
    wx.humid.inlo.hour = 10 * LO(buf[16]) + HI(buf[15]);
    wx.humid.inlo.day = 10 * LO(buf[17]) + HI(buf[16]);
    wx.humid.inlo.mon = HI(buf[17]);
    wx.humid.ina.nhi = wx.humid.ina.hi = NUM(buf[18]);
    wx.humid.ina.nlo = wx.humid.ina.lo = NUM(buf[19]);
    ERROR(wx.humid.out, buf[20], NUM(buf[20]));
    wx.humid.out.n = wx.humid.out.val;
    wx.humid.outhi.n = wx.humid.outhi.val = NUM(buf[21]);
    wx.humid.outhi.min = NUM(buf[22]);
    wx.humid.outhi.hour = NUM(buf[23]);
    wx.humid.outhi.day = NUM(buf[24]);
    wx.humid.outhi.mon = LO(buf[25]);
    wx.humid.outlo.n = wx.humid.outlo.val
      = 10 * LO(buf[26]) + HI(buf[25]);
    wx.humid.outlo.min = 10 * LO(buf[27]) + HI(buf[26]);
    wx.humid.outlo.hour = 10 * LO(buf[28]) + HI(buf[27]);
    wx.humid.outlo.day = 10 * LO(buf[29]) + HI(buf[28]);
    wx.humid.outlo.mon = HI(buf[29]);
    wx.humid.outa.nhi = wx.humid.outa.hi = NUM(buf[30]);
    wx.humid.outa.nlo = wx.humid.outa.lo = NUM(buf[31]);
    wx.humid.outhi.or = BIT(buf[32], 3);
    wx.humid.out.or = BIT(buf[32], 4);
    wx.humid.inhi.or = BIT(buf[32], 6);
    wx.humid.in.or = BIT(buf[32], 7);
    wx.unk.eightf32 = buf[32] & 0x27;
    wx.unk.eightf33 = buf[33] & 0x07;
    wx.td.a.on = BIT(buf[33], 3);
    wx.humid.ina.on = buf[33] & 0xc0 ? 1 : 0;
    wx.humid.outa.on = buf[33] & 0x30 ? 1 : 0;
    ret = 33;
    break;
  case GROUP9:			/* Temperature */
    ERROR(wx.temp.in, buf[1], (buf[2] & 0x08 ? -1 : 1)
	  * (100 * (buf[2] & 0x07) + NUM(buf[1])));
    wx.temp.in.n = TENTHS(wx.temp.in.val);
    wx.temp.inhi.val = (buf[3] & 0x80 ? -1 : 1)
      * (10 * NUM(buf[3] & 0x7f) + HI(buf[2]));
    wx.temp.inhi.n = TENTHS(wx.temp.inhi.val);
    wx.temp.inhi.min = NUM(buf[4]);
    wx.temp.inhi.hour = NUM(buf[5]);
    wx.temp.inhi.day = NUM(buf[6]);
    wx.temp.inhi.mon = LO(buf[7]);
    wx.temp.inlo.val = (buf[8] & 0x80 ? -1 : 1)
      * (10 * NUM(buf[8] & 0x7f) + HI(buf[7]));
    wx.temp.inlo.n = TENTHS(wx.temp.inlo.val);
    wx.temp.inlo.min = NUM(buf[9]);
    wx.temp.inlo.hour = NUM(buf[10]);
    wx.temp.inlo.day = NUM(buf[11]);
    wx.temp.inlo.mon = LO(buf[12]);
    wx.unk.ninef13 = buf[13] & 0xe0;
    wx.temp.ina.hi = 10 * NUM(buf[13]) + HI(buf[12]);
    wx.temp.ina.nhi = F2C(wx.temp.ina.hi);
    wx.temp.ina.lo = 100 * BIT(buf[15], 0) + NUM(buf[14]);
    wx.temp.ina.nlo = F2C(wx.temp.ina.lo);
    wx.temp.format = BIT(buf[15], 6);
    wx.unk.ninef15 = buf[15] & 0xbe;
    ERROR(wx.temp.out, buf[16], (buf[17] & 0x08 ? -1 : 1)
	  * (100 * (buf[17] & 0x07) + NUM(buf[16])));
    wx.temp.out.n = TENTHS(wx.temp.out.val);
    wx.temp.outhi.val = (buf[18] & 0x80 ? -1 : 1)
      * (10 * NUM(buf[18] & 0x7f) + HI(buf[17]));
    wx.temp.outhi.n = TENTHS(wx.temp.outhi.val);
    wx.temp.outhi.min = NUM(buf[19]);
    wx.temp.outhi.hour = NUM(buf[20]);
    wx.temp.outhi.day = NUM(buf[21]);
    wx.temp.outhi.mon = LO(buf[22]);
    wx.temp.outlo.val = (buf[23] & 0x80 ? -1 : 1)
      * (10 * NUM(buf[23] & 0x7f) + HI(buf[22]));
    wx.temp.outlo.n = TENTHS(wx.temp.outlo.val);
    wx.temp.outlo.min = NUM(buf[24]);
    wx.temp.outlo.hour = NUM(buf[25]);
    wx.temp.outlo.day = NUM(buf[26]);
    wx.temp.outlo.mon = LO(buf[27]);
    wx.unk.ninef28 = buf[28] & 0x60;
    wx.temp.outa.hi = (buf[28] & 0x80 ? -1 : 1)
      * (10 * NUM(buf[28] & 0x1f) + HI(buf[27]));
    wx.temp.outa.nhi = F2C(wx.temp.outa.hi);
    wx.temp.outa.lo = (buf[30] & 0x08 ? -1 : 1)
      * (100 * BIT(buf[30], 0) + NUM(buf[29]));
    wx.temp.outa.nlo = F2C(wx.temp.outa.lo);
    wx.unk.ninef30 = buf[30] & 0xf6;
    wx.unk.ninef31 = buf[31];
    wx.temp.ina.on = buf[33] & 0xc0 ? 1 : 0;
    wx.temp.outa.on = buf[33] & 0x30 ? 1 : 0;
    wx.temp.ina.on = buf[32] & 0xc0 ? 1 : 0;
    wx.temp.outa.on = buf[32] & 0x30 ? 1 : 0;
    ret = 32;
    break;
  case GROUPA:			/* Barometer, Dew Point */
    ERROR(wx.baro.local, buf[2], 1000 * NUM(buf[2]) + 10 * NUM(buf[1]));
    wx.baro.local.n = TENTHS(wx.baro.local.val);
    ERROR(wx.baro.sea, buf[5],
	  10000 * LO(buf[5]) + 100 * NUM(buf[4]) + NUM(buf[3]));
    wx.baro.sea.n = TENTHS(wx.baro.sea.val);
    wx.baro.format = HI(buf[5] & 0x30);
    wx.unk.af5 = buf[5] & 0xc0;
    wx.baro.pred = LO(buf[6]);
    wx.baro.trend = HI(buf[6] & 0x70);
    wx.unk.af6 = buf[6] & 0x80;

    ERROR(wx.dew.in, buf[7], NUM(buf[7]));
    wx.dew.in.n = wx.dew.in.val;
    wx.dew.inhi.n = wx.dew.inhi.val = NUM(buf[8]);
    wx.dew.inhi.min = NUM(buf[9]);
    wx.dew.inhi.hour = NUM(buf[10]);
    wx.dew.inhi.day = NUM(buf[11]);
    wx.dew.inhi.mon = LO(buf[12]);
    wx.dew.inlo.n = wx.dew.inlo.val = 10 * LO(buf[13]) + HI(buf[12]);
    wx.dew.inlo.min = 10 * LO(buf[14]) + HI(buf[13]);
    wx.dew.inlo.hour = 10 * LO(buf[15]) + HI(buf[14]);
    wx.dew.inlo.day = 10 * LO(buf[16]) + HI(buf[15]);
    wx.dew.inlo.mon = HI(buf[16]);
    wx.dew.ina.nlo = wx.dew.ina.lo = LO(buf[17]) + 1;
    wx.dew.outa.nlo = wx.dew.outa.nlo = HI(buf[17]) + 1;
    ERROR(wx.dew.out, buf[18], NUM(buf[18]));
    wx.dew.out.n = wx.dew.out.val;
    wx.dew.outhi.n = wx.dew.outhi.val = NUM(buf[19]);
    wx.dew.outhi.min = NUM(buf[20]);
    wx.dew.outhi.hour = NUM(buf[21]);
    wx.dew.outhi.day = NUM(buf[22]);
    wx.dew.outhi.mon = LO(buf[23]);
    wx.dew.outlo.n = wx.dew.outlo.val = 10 * LO(buf[24]) + HI(buf[23]);
    wx.dew.outlo.min = 10 * LO(buf[25]) + HI(buf[24]);
    wx.dew.outlo.hour = 10 * LO(buf[26]) + HI(buf[25]);
    wx.dew.outlo.day = 10 * LO(buf[27]) + HI(buf[26]);
    wx.dew.outlo.mon = HI(buf[27]);
    wx.dew.outlo.or = BIT(buf[28], 0);
    wx.dew.out.or = BIT(buf[28], 2);
    wx.dew.inlo.or = BIT(buf[28], 3);
    wx.dew.in.or = BIT(buf[28], 5);
    wx.unk.af28 = buf[28] & 0xd2;
    wx.dew.ina.on = wx.dew.outa.on = buf[29] & 0x60 ? 1 : 0;
    wx.baro.a.on = BIT(buf[29], 7);
    wx.baro.a.nhi = wx.baro.a.hi = LO(buf[29]) + 1;
    wx.unk.af29 = buf[29] & 0x10;
    ret = 29;
    break;
  case GROUPB:			/* Rain */
    ERROR(wx.rain.rate, buf[1], 100 * LO(buf[2]) + NUM(buf[1]));
    wx.rain.rate.n = wx.rain.rate.val;
    wx.unk.bf2 = buf[2] & 0xf0;
    wx.rain.yest.n = wx.rain.yest.val = 100 * NUM(buf[4]) + NUM(buf[3]);
    wx.rain.total.n = wx.rain.total.val = 100 * NUM(buf[6]) + NUM(buf[5]);
    wx.rain.total.min = NUM(buf[7]);
    wx.rain.total.hour = NUM(buf[8]);
    wx.rain.total.day = NUM(buf[9]);
    wx.rain.total.mon = LO(buf[10]);
    wx.unk.bf10 = buf[10] & 0xd0;
    wx.rain.format = BIT(buf[10], 5);
    wx.rain.a.hi = 100 * LO(buf[12]) + NUM(buf[11]);
    wx.rain.a.nhi = IN2MM(TENTHS(wx.rain.a.hi));
    wx.rain.a.on = BIT(buf[12], 4);
    wx.rain.rate.or = BIT(buf[12], 7);
    wx.unk.bf12 = buf[12] & 0x60;
    ret = 12;
    break;
  case GROUPC:			/* Wind, Wind Chill, General */
    ERROR(wx.wind.gspeed, buf[1], 100 * LO(buf[2]) + NUM(buf[1]));
    wx.wind.gspeed.n = TENTHS(wx.wind.gspeed.val);
    wx.wind.gdir.n = wx.wind.gdir.val = 10 * NUM(buf[3]) + HI(buf[2]);
    ERROR(wx.wind.aspeed, buf[4], 100 * LO(buf[5]) + NUM(buf[4]));
    wx.wind.aspeed.n = TENTHS(wx.wind.aspeed.val);
    wx.wind.adir.n = wx.wind.adir.val = 10 * NUM(buf[6]) + HI(buf[5]);
    wx.wind.speedhi.val = 100 * LO(buf[8]) + NUM(buf[7]);
    wx.wind.speedhi.n = TENTHS(wx.wind.speedhi.val);
    wx.wind.dirhi.n = wx.wind.dirhi.val = 10 * NUM(buf[9]) + HI(buf[8]);
    wx.wind.speedhi.min = NUM(buf[10]);
    wx.wind.speedhi.hour = NUM(buf[11]);
    wx.wind.speedhi.day = NUM(buf[12]);
    wx.wind.speedhi.mon = LO(buf[13]);
    wx.wind.a.hi = (buf[14] & 0x10 ? 100 : 0)
      + 10 * LO(buf[14]) + HI(buf[13]);
    wx.wind.a.nhi = MPH2MPS(wx.wind.a.hi);
    wx.unk.cf14 = buf[14] & 0xe0;
    wx.unk.cf15 = buf[15] & 0x3f;
    wx.wind.format = (buf[15] & 0xc0) >> 6;
    ERROR(wx.chill.temp, buf[16], (buf[21] & 0x20 ? -1 : 1)
	  * NUM(buf[16]));
    wx.chill.temp.n = wx.chill.temp.val;
    wx.chill.low.n = wx.chill.low.val = (buf[21] & 0x10 ? -1 : 1)
      * NUM(buf[17]);
    wx.chill.low.min = NUM(buf[18]);
    wx.chill.low.hour = NUM(buf[19]);
    wx.chill.low.day = NUM(buf[20]);
    wx.chill.low.mon = LO(buf[21]);
    wx.unk.cf21 = buf[21] & 0xc0;
    wx.chill.a.lo = (buf[23] & 0x08 ? -1 : 1)
      * (100 * HI(buf[23] & 0x10) + NUM(buf[22]));
    wx.chill.a.nlo = F2C(wx.chill.a.lo);
    wx.unk.cf23 = buf[23] & 0x27;
    wx.gen.power = BIT(buf[23], 6);
    wx.gen.lowbat = BIT(buf[23], 7);
    wx.gen.section = HI(buf[24] & 0x70);
    wx.gen.screen = (buf[24] & 0x0c) >> 2;
    wx.gen.subscreen = buf[24] & 0x03;
    wx.unk.cf24 = buf[24] & 0x80;
    wx.chill.a.on = BIT(buf[25], 1);
    wx.wind.a.on = BIT(buf[25], 2);
    wx.wind.speedhi.or = BIT(buf[25], 5);
    wx.wind.aspeed.or = BIT(buf[25], 6);
    wx.wind.gspeed.or = BIT(buf[25], 7);
    wx.unk.cf25 = buf[25] & 0x19;
    ret = 25;
    break;
  }
  return(ret);
}
