/*
 * @(#)$Id: tab.c,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file parses the data from groupbuf into tab-delimited ASCII tabbuf.
 *
 */

#include <stdio.h>
#include <string.h>
#include "wx200.h"

char tabbuf[TABLEN];

int _tabchanged = 0;
int _tablast = -1;
char _tabprev[TABLEN];

void
wx200tabinit() {
  _tabchanged = 0;
  _tablast = -1;
  _tabprev[0] = '\0';
}

/* call wx200parse(group), returning whether tabbuf changed */
int
wx200tab(int group) {
  static int ret;
  static char buf[TABLEN];
  static float time = 0;

  ret = 0;
  if (group >= 0) {		/* GROUPC,F,8 indicate end of previous record */
    if (group == GROUPC || group == GROUPF || group == GROUP8) {
      if (group == GROUPC) {
	_tabchanged = 1;	/* handle a GROUPC change now */
	wx200parse(group);
	if (_tablast == GROUPC) time += 5.0 / 3600.0; /* add 5 seconds? */
      }
      if (_tabchanged) {	/* did something change in previous record? */
	sprintf(buf,
		"%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%d\t%g\t%d\t%g\t%g\t%g\t%g",
		wx.temp.in.n, wx.temp.out.n,
		wx.humid.in.n, wx.humid.out.n,
		wx.dew.in.n, wx.dew.out.n,
		wx.baro.local.n, wx.baro.sea.n,
		unit_dir(wx.wind.gdir.n, 36) * 10, wx.wind.gspeed.n,
		unit_dir(wx.wind.adir.n, 36) * 10, wx.wind.aspeed.n,
		wx.chill.temp.n, wx.rain.rate.n, wx.rain.total.n);
	if (strncmp(_tabprev, buf, TABLEN)) { /* a change we care about? */
	  sprintf(tabbuf, "%.4f\t%s\n", time, buf); /* yes, export it */
	  ret = 1;
	  strncpy(_tabprev, buf, TABLEN); /* for comparison next time */
	}
      }
      _tabchanged = group == GROUP8 ? 1 : 0; /* C handled, F don't care */
    } else {			/* GROUP9,A,B changed */
      _tabchanged = 1;
    }
    if (group != GROUPC) {	/* C handled and printed already */
      wx200parse(group);	/* update data and clock */
      time = wx.td.clock.hour + wx.td.clock.min
	/ 60.0 + wx.td.clock.sec / 3600.0;
    }
    _tablast = group;
  }
  return(ret);
}
