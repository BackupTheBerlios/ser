/*
 * @(#)$Id: format.c,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements unit labels and text formatting.
 *
 */

#include <stdio.h>
#include <string.h>
#include "wx200.h"

int wx200ignoreflags = 0;
int wx200ignoreunits = 0;

WX_FORMAT wxformat = {
  {{2, 0, "%", "Percent"}},	/* humid */

  {{5, 1, "C", "Degrees Celsius"}, /* temp */
   {5, 1, "F", "Degrees Fahrenheit"}},

  {{4, 0, "C", "Degrees Celsius"}, /* temp1 */
   {4, 0, "F", "Degrees Fahrenheit"},
   {4, 0, "F", "Degrees Fahrenheit"}},

  {{5, 2, "in ", "Inches of Mercury"}, /* press */
   {5, 1, "mm ", "Millimeters of Mercury"},
   {5, 0, "mb ", "Millibars"},
   {5, 0, "hpa", "Hecto-Pascals"}},

  {{5, 0, "mm", "Millimeters"},	/* depth */
   {5, 2, "in", "Inches"}},

  {{5, 0, "mm/h", "Millimeters per Hour"}, /* rate */
   {5, 2, "in/h", "Inches per Hour"}},

  {{5, 1, "mph  ", "Miles per Hour"}, /* speed */
   {5, 1, "knots", "Knots"},
   {5, 1, "m/s  ", "Meters per Second"},
   {5, 1, "kph  ", "Kilometers per Hour"}},

  {"N", "E", "S", "W",},	/* wind4 */
  {"N ", "NE", "E ", "SE", "S ", "SW", "W ", "NW"}, /* wind8 */
  {" N ", "NNE", "NE ", "ENE", " E ", "ESE", "SE ", "SSE", /* wind16 */
   " S ", "SSW", "SW ", "WSW", " W ", "WNW", "NW ", "NNW"}
};

char
unit_flag(int err, int or)
{
  char flag_unit[] = " *!#";
  if (wx200ignoreflags)
    return(' ');
  return(flag_unit[((err ? 1 : 0) << 1) | (or ? 1 : 0)]);
}

void
unit_format(char *buf, WX_SENS *sensormem, int format,
	    WX_UNIT *unit, float (*convert)(float, int))
{
  int p, w;
  float n;
  char numbuf[10];

  w = unit[format].w;
  p = unit[format].p;
  n = convert(sensormem->n, format);
  do {				/* drop decimals if too wide */
    sprintf(numbuf, "%*.*f", w, p--, n);
  } while (p >= 0 && strlen(numbuf) > w);
  sprintf(buf, "%s%c%s", numbuf,
	  unit_flag(sensormem->err, sensormem->or),
	  wx200ignoreunits ? "" : unit[format].l);
}

char *
format_data(int what, char *buf, WX_SENS *sensormem, int format)
{
  switch (what) {
  case F_HUMID:
    unit_format(buf, sensormem, format, wxformat.humid, &unit_humid);
    break;
  case F_TEMP:
    unit_format(buf, sensormem, format, wxformat.temp, &unit_temp);
    break;
  case F_TEMP1:
    unit_format(buf, sensormem, format, wxformat.temp1, &unit_temp);
    break;
  case F_DIFF1:
    unit_format(buf, sensormem, format ? 2 : 0, wxformat.temp1, &unit_temp);
    break;
  case F_PRESS:
    unit_format(buf, sensormem, format, wxformat.press, &unit_press);
    break;
  case F_DEPTH:
    unit_format(buf, sensormem, format, wxformat.depth, &unit_depth);
    break;
  case F_RATE:
    unit_format(buf, sensormem, format, wxformat.rate, &unit_depth);
    break;
  case F_SPEED:
    unit_format(buf, sensormem, format, wxformat.speed, &unit_speed);
    break;
  case F_DIR:
    sprintf(buf, "%03d%s", sensormem->val, wx200ignoreunits ? ""
	    : wxformat.wind16[unit_dir(sensormem->n, 16)]);
    break;
  case F_TIME:
    sprintf(buf, "%2d/%2d %2d:%02d%s",
	    format & 0x02 ? sensormem->mon : sensormem->day,
	    format & 0x02 ? sensormem->day : sensormem->mon,
	    format & 0x01 ? sensormem->hour
	    : (sensormem->hour % 12 ? sensormem->hour % 12 : 12),
	    sensormem->min,
	    format & 0x01 ? "  " : (sensormem->hour >= 12 ? "pm" : "am"));
    break;
  }
  return(buf);
}

char *
format_val(int what, char *buf, float val, int format)
{
  WX_SENS t;

  t.err = t.or = 0;
  t.n = t.val = val;
  format_data(what, buf, &t, format);
  return(buf);
}
