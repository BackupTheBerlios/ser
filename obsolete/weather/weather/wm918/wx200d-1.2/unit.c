/*
 * @(#)$Id: unit.c,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements float unit conversions.
 *
 */

#include "wx200.h"

float
unit_humid(float percent, int unit)
{
  return(percent);
}

float
unit_temp(float celsius, int unit)
{
  switch (unit) {
  case 0:			/* Celsius */
    return(celsius);
  case 1:			/* Fahrenheit */
    return(32.0 + 1.8 * celsius);
  case 2:			/* Fahrenheit diff (dew point) */
    return(1.8 * celsius);
  }
  return(celsius);
}

float
unit_press(float millibars, int unit)
{
  switch (unit) {
  case 0:			/* inHg */
    return(0.029529987508 * millibars);
  case 1:			/* mmHg */
    return(0.750061682704 * millibars);
  case 2:			/* mbar */
    return(millibars);
  case 3:			/* hPa */
    return(millibars);		/* 1000 mbar = 1 bar = 100000 Pa = 1000 hPa */
  }
  return(millibars);
}

float
unit_depth(float mm, int unit)
{
  switch (unit) {
  case 0:			/* mm */
    return(mm);
  case 1:			/* in */
    return(0.0393700787402 * mm);
  }
  return(mm);
}

float
unit_speed(float mps, int unit)
{
  switch (unit) {
  case 0:			/* mph */
    return(2.23693629205 * mps);
  case 1:			/* knots */
    return(1.94384449244 * mps);
  case 2:			/* mps */
    return(mps);
  case 3:			/* kph */
    return(3.6 * mps);
  }
  return(mps);
}

int
unit_dir(float dir, int div)	/* wind direction divider */
{
  float per;

  per = 360.0 / div;
  return((int)((dir + (per / 2.0)) / per) % div);
}
