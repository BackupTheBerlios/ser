/*
 * @(#)$Id: wx200.c,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements a command-line wx200d client
 *
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include "wx200.h"

#define MAXARGS 20		/* maximum CGI args to parse */
#define TMPLEN 128		/* max length of CGI */

char *progname;
char *query;

void
usage(int ret)
{
  if (query)
      printf("Content-type: text/plain\nStatus: 200\n\n");
  printf("usage: %s [options] [[host][:port]]
  -h	--help			show this help and exit		VERSION: %s
  -r	--refresh		continuously refresh instead of exiting
  --dm --md --12hr --24hr		date order and time format
  --C --celsius --F --fahrenheit	temperature format
  --inhg --mmhg --mbar --hpa		barometric pressure format
  --mm --mm/h --in --in/h		rain rate and accumulation format
  --mph --knots --mps --kph		wind speed format
  -e	--noerrors		suppress error and out of range flags
  -u	--nounits		suppress units from the display
  -a	--alarms		add all alarms and legend to the display  OR:
  -t	--tab			tab-delimited metric line as logged	  OR:
  -l	--line			all variables on one line in custom units OR:
	--aprs			line in APRS (ham radio) format and units OR:
  --intemp --inhumidity --indewpoint	show indoor variable(s)
  --temp --humidity --dewpoint		show outdoor variable(s)
  --local --sea				show barometric pressure(s)
  --gust --average --chill		show wind variable(s)
  --rate --yesterday --total		show rain variable(s)
  --trend --forecast			show barometer trend / 12-24hr forecast
  --power --battery --display		show power, battery, display status
options may be uniquely abbreviated; units default to those of the WX200
", progname, VERSION);
  exit(ret);
}

int
main(int argc, char **argv)
{
  const char *flags = "Hharlteu";
  int opt, num = EOF, socket, done = 0, opt_a = 0, opt_r = 0, opt_t = 0;
  int index, ftemp = -1, fbaro = -1, fdepth = -1, fspeed = -1, ftd = 0;
  int line = 0, i;
  char *hostport = NULL, buf[300], *string, *arg[MAXARGS + 2];
  char *tmp, tmpbuf[TMPLEN];
  char *disp[] = {"Clock", "Temp", "Humidity", "Dew",
		  "Baro", "Wind", "W.Chill", "Rain"};
  char *acdc[] = {"AC", "DC"};
  char *batt[] = {"ok ", "LOW"};
  char format[] = "%11s:"
    "%7s Hi%7s %s %7s Hi%7s %s\n"
    "            "
    "        Lo%7s %s         Lo%7s %s\n";
  char aformat[] = "                    "
    "Al%7s %7s %3s           Al%7s %7s %3s\n\n";
  char *trend[] = {"Rising", "Steady", "", "Falling"};
  char *pred[] = {"Sunny", "Cloudy", "", "Partly Cloudy",
		  "", "", "", "Rain"};
  char *offon[] = {"Off", "On "};
  int current[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  struct option longopt[] = {
    {"help", 0, 0, 'h'},
    {"alarms", 0, 0, 'a'},
    {"refresh", 0, 0, 'r'},
    {"line", 0, 0, 'l'},
    {"tab", 0, 0, 't'},
    {"noerrors", 0, 0, 'e'},
    {"nounits", 0, 0, 'u'},
    {"celsius", 0, 0, 1}, {"fahrenheit", 0, 0, 2},
    {"C", 0, 0, 1}, {"F", 0, 0, 2},
    {"inhg", 0, 0, 3}, {"mmhg", 0, 0, 4}, {"mbar", 0, 0, 5}, {"hpa", 0, 0, 6},
    {"mm", 0, 0, 7}, {"mm/h", 0, 0, 7}, {"in", 0, 0, 8}, {"in/h", 0, 0, 8},
    {"mph", 0, 0, 9}, {"knots", 0, 0, 10}, {"mps", 0, 0, 11}, {"kph", 0, 0, 12},
    {"dm", 0, 0, 13}, {"md", 0, 0, 14},
    {"12hr", 0, 0, 15}, {"24hr", 0, 0, 16},
    {"aprs", 0, &current[0], 2},

    /* added by trs: 12 Feb 1998, names changed by twitham 16 Apr 1998 */
    {"intemp", 0, &current[0], 1},
    {"temp", 0, &current[1], 1}, /* can now  do multiple ones 9 Jun 2001 */
    {"inhumidity", 0, &current[2], 1},
    {"humidity", 0, &current[3], 1},
    {"indewpoint", 0, &current[4], 1},
    {"dewpoint", 0, &current[5], 1},
    {"local", 0, &current[6], 1},
    {"sea", 0, &current[7], 1},
    {"gust", 0, &current[8], 1},
    {"average", 0, &current[9], 1},
    {"chill", 0, &current[10], 1},
    {"rate", 0, &current[11], 1},
    {"yesterday", 0, &current[12], 1},
    {"total", 0, &current[13], 1},
    {"trend", 0, &current[14], 1},
    {"forecast", 0, &current[15], 1},
    {"power", 0, &current[16], 1},
    {"battery", 0, &current[17], 1},
    {"display", 0, &current[18], 1},
    {0, 0, 0, 0}
  };

  progname = strrchr(argv[0], '/');
  if (progname == NULL)
    progname = argv[0];
  else
    progname++;

  if ((query = getenv("QUERY_STRING"))) { /* we running as a web CGI ? */
    strncpy(tmpbuf, query, TMPLEN);
    tmp = tmpbuf;
    argv = arg;			/* get args from CGI env */
    argv[argc = 0] = progname;
    while ((argc < MAXARGS) && (string = strtok(argc ? NULL : tmp, "&="))) {
      if (string[0] != '\0') {
	argc++;
	argv[argc] = string;
      }
    }
    argv[++argc] = NULL;
  }

  while ((opt = getopt_long(argc, argv, flags, longopt, &index)) != EOF) {
    switch (opt) {		/* parse command-line or CGI options */
    case '?':
      usage(1);
    case 'h':
    case 'H':
      usage(0);
    case 0:
      break;
    case 1:
    case 2:
      ftemp = opt - 1;
      break;
    case 3:
    case 4:
    case 5:
    case 6:
      fbaro = opt - 3;
      break;
    case 7:
    case 8:
      fdepth = opt - 7;
      break;
    case 9:
    case 10:
    case 11:
    case 12:
      fspeed = opt - 9;
      break;
    case 13:
    case 14:			/* bit 7 is flag, bit 3 is value */
      ftd |= 0x80 | ((opt - 13) << 3);
      break;
    case 15:
    case 16:			/* bit 6 is flag, bit 2 is value */
      ftd |= 0x40 | ((opt - 15) << 2);
      break;
    case 'a':
      opt_a = 1;
      break;
    case 'r':
      opt_r = 1;
      break;
    case 'l':
      line = 1;
      break;
    case 't':
      opt_t = 1;
      break;
    case 'e':
      wx200ignoreflags = 1;
      break;
    case 'u':
      wx200ignoreunits = 1;
      break;
    }
  }
  if (line)			/* all vars on a line? */
    for (i = 0; i < 14; i++) {
      current[i] = 1;
    }
  for (i = 0; i < 19; i++) {	/* custom line mode? */
    if (current[i]) line = 1;
  }
  if (optind < argc && argv[optind] != NULL) {
    hostport = argv[optind];
    if ((tmp = strchr(hostport, '%')) != NULL) { /* CGI hack %3A -> : */
      if (*(tmp + 1) == '3' && (*(tmp + 2) == 'A' || *(tmp + 2) == 'a'))
	*tmp = *(tmp + 1) = *(tmp + 2) = ':';
    }
  }

  if ((socket = wx200open(hostport)) < 0) {
    fprintf(stderr, "%s\n", wx200error);
    exit(socket);
  }
  if (query) {
    printf("Content-type: text/html\n");
    if (opt_r)
      printf("Refresh: 10; URL=%s?%s\n",
	     (tmp = getenv("SCRIPT_NAME")) ? tmp : "", query);
    opt_r = 0;
    printf("Status: 200

<html><head>
<title>%s @ %s:%d</title>
</head><body>
<h1>%s @ %s:%d</h1>
<hr>
<pre>
", progname, wx200host, wx200port, progname, wx200host, wx200port);
  }

  wx200bufinit();		/* get current conditions first */
  while ((done < 2) && ((num = wx200bufread(socket)) != EOF)) {
    wx200tab(num);		/* calls wx200parse(num) */
    if ((num == GROUP8) || (num == GROUPF)) done++;
  }

  if (ftemp == -1) ftemp = wx.temp.format; /* prefs override wx200 settings */
  if (fbaro == -1) fbaro = wx.baro.format;
  if (fdepth == -1) fdepth = wx.rain.format;
  if (fspeed == -1) fspeed = wx.wind.format;
  if (ftd & 0x80) ftd |= (ftd & 0x08) >> 2; /* m/d preference? */
  else ftd |= wx.td.format & 0x02;
  if (ftd & 0x40) ftd |= (ftd & 0x04) >> 2; /* 12/24 preference? */
  else ftd |= wx.td.format & 0x01;

  do {				/* forever if -r or --refresh, once otherwise */
    wx200tab(num);		/* calls wx200parse(num) */

    format_data(F_TIME, buf, &wx.td.clock, ftd); /* hack to add seconds */
    strcpy(buf + 20, buf + 11);			/* save am/pm to temp area */
    sprintf(buf + 11, ":%02d", wx.td.clock.sec); /* overwrite with seconds */
    strcpy(buf + 14, buf + 20);			/* append am/pm */
    sprintf(buf + 100, "%2d:%02d%s", ftd & 0x01 ? wx.td.a.hi
	    : (wx.td.a.hi % 12 ? wx.td.a.hi % 12 : 12), wx.td.a.lo,
	    ftd & 0x01 ? "  " : (wx.td.a.hi >= 12 ? "pm" : "am"));

    if (opt_t) {		/* tab-delimited like log mode */
      printf("%s", tabbuf);
      fflush(stdout);
      continue;			/* skip the rest of the loop */
    }

    /* APRS mode contributed by Steve Fraser <sfraser@sierra.apana.org.au> */
    if (current[0] == 2) {
      if (unit_temp(wx.temp.out.n,1) < 0) { /* only put sign if negative */
	sprintf(tmpbuf,"%+02.0f",unit_temp(wx.temp.out.n,1));
      } else {
	sprintf(tmpbuf,"%03.0f",unit_temp(wx.temp.out.n,1));
      }
      printf("%03.0f/%03.0fg%03.0ft%sr%03.0fp%03.0fh%02ib%05.0fXRSW\n",
	     wx.wind.adir.n,
	     unit_speed(wx.wind.aspeed.n,0),
	     unit_speed(wx.wind.gspeed.n,0),
	     tmpbuf,
	     unit_depth(wx.rain.rate.n,1) * 100,
	     unit_depth(wx.rain.yest.n,1) * 100,
	     (int) wx.humid.out.n % 100,
	     wx.baro.local.n * 10);
      fflush(stdout);
      continue;			/* skip the rest of the loop */
    }

    if (line) {			/* custom line mode */
      for (i = 0; i < 19; i++) {
	if (current[i]) {
	  switch (i) {
	  case 0:
	    printf("%s\t", format_data(F_TEMP, buf, &wx.temp.in, ftemp));
	    break;
	  case 1:
	    printf("%s\t", format_data(F_TEMP, buf, &wx.temp.out, ftemp));
	    break;
	  case 2:
	    printf("%s\t", format_data(F_HUMID, buf, &wx.humid.in, 0));
	    break;
	  case 3:
	    printf("%s\t", format_data(F_HUMID, buf, &wx.humid.out, 0));
	    break;
	  case 4:
	    printf("%s\t", format_data(F_TEMP1, buf, &wx.dew.in, ftemp));
	    break;
	  case 5:
	    printf("%s\t", format_data(F_TEMP1, buf, &wx.dew.out, ftemp));
	    break;
	  case 6:
	    printf("%s\t", format_data(F_PRESS, buf, &wx.baro.local, fbaro));
	    break;
	  case 7:
	    printf("%s\t", format_data(F_PRESS, buf, &wx.baro.sea, fbaro));
	    break;
	  case 8:
	    printf("%s\t%s\t",
		   format_data(F_SPEED, buf, &wx.wind.gspeed, fspeed),
		   format_data(F_DIR, buf + 20, &wx.wind.gdir, 0));
	    break;
	  case 9:
	    printf("%s\t%s\t",
		   format_data(F_SPEED, buf, &wx.wind.aspeed, fspeed),
		   format_data(F_DIR, buf + 20, &wx.wind.adir, 0));
	    break;
	  case 10:
	    printf("%s\t", format_data(F_TEMP1, buf, &wx.chill.temp, ftemp));
	    break;
	  case 11:
	    printf("%s\t", format_data(F_RATE, buf, &wx.rain.rate, fdepth));
	    break;
	  case 12:
	    printf("%s\t", format_data(F_DEPTH, buf, &wx.rain.yest, fdepth));
	    break;
	  case 13:
	    printf("%s\t", format_data(F_DEPTH, buf, &wx.rain.total, fdepth));
	    break;
	  case 14:
	    printf("%s\t", trend[wx.baro.trend - 1]);
	    break;
	  case 15:
	    printf("%s\t", pred[wx.baro.pred - 1]);
	    break;
	  case 16:
	    printf("%s\t", acdc[wx.gen.power]);
	    break;
	  case 17:
	    printf("%s\t", batt[wx.gen.lowbat]);
	    break;
	  case 18:
	    printf("%8s/%d/%d\t",
		   disp[wx.gen.section], wx.gen.screen, wx.gen.subscreen);
	    break;
	  }
	}
      }
      printf("\n");
      fflush(stdout);
      continue;			/* skip the rest of the loop */
    }

    printf("WX200 %xf: %20s:%-5d %s", num, wx200host, wx200port, buf);
    if (opt_a)
      printf("    Alarm: %s %s", buf + 100, offon[wx.td.a.on]);
    printf("\n%5s             Display: %8s/%d/%d"
	   "          Power: %s  Battery: %s\n"
	   "             "
	   "---------- Outdoor -----------  ----------- Indoor -----------\n",
	   VERSION, disp[wx.gen.section], wx.gen.screen, wx.gen.subscreen,
	   acdc[wx.gen.power], batt[wx.gen.lowbat]);

    printf(format, "Temperature",
	   format_data(F_TEMP, buf + 60, &wx.temp.out, ftemp),
	   format_data(F_TEMP, buf + 80, &wx.temp.outhi, ftemp),
	   format_data(F_TIME, buf + 100, &wx.temp.outhi, ftd),
	   format_data(F_TEMP, buf, &wx.temp.in, ftemp),
	   format_data(F_TEMP, buf + 20, &wx.temp.inhi, ftemp),
	   format_data(F_TIME, buf + 40, &wx.temp.inhi, ftd),
	   format_data(F_TEMP, buf + 160, &wx.temp.outlo, ftemp),
	   format_data(F_TIME, buf + 180, &wx.temp.outlo, ftd),
	   format_data(F_TEMP, buf + 120, &wx.temp.inlo, ftemp),
	   format_data(F_TIME, buf + 140, &wx.temp.inlo, ftd));
    if (opt_a)
      printf(aformat,
	     format_val(F_TEMP1, buf + 240, wx.temp.outa.nlo, ftemp),
	     format_val(F_TEMP1, buf + 260, wx.temp.outa.nhi, ftemp),
	     offon[wx.temp.outa.on],
	     format_val(F_TEMP1, buf + 200, wx.temp.ina.nlo, ftemp),
	     format_val(F_TEMP1, buf + 220, wx.temp.ina.nhi, ftemp),
	     offon[wx.temp.ina.on]);

    printf(format, "Humidity",
	   format_data(F_HUMID, buf + 60, &wx.humid.out, 0),
	   format_data(F_HUMID, buf + 80, &wx.humid.outhi, 0),
	   format_data(F_TIME, buf + 100, &wx.humid.outhi, ftd),
	   format_data(F_HUMID, buf, &wx.humid.in, 0),
	   format_data(F_HUMID, buf + 20, &wx.humid.inhi, 0),
	   format_data(F_TIME, buf + 40, &wx.humid.inhi, ftd),
	   format_data(F_HUMID, buf + 160, &wx.humid.outlo, 0),
	   format_data(F_TIME, buf + 180, &wx.humid.outlo, ftd),
	   format_data(F_HUMID, buf + 120, &wx.humid.inlo, 0),
	   format_data(F_TIME, buf + 140, &wx.humid.inlo, ftd));
    if (opt_a)
      printf(aformat,
	     format_val(F_HUMID, buf + 240, wx.humid.outa.nlo, 0),
	     format_val(F_HUMID, buf + 260, wx.humid.outa.nhi, 0),
	     offon[wx.humid.outa.on],
	     format_val(F_HUMID, buf + 200, wx.humid.ina.nlo, 0),
	     format_val(F_HUMID, buf + 220, wx.humid.ina.nhi, 0),
	     offon[wx.humid.ina.on]);

    printf(format, "Dew Point",
	   format_data(F_TEMP1, buf + 60, &wx.dew.out, ftemp),
	   format_data(F_TEMP1, buf + 80, &wx.dew.outhi, ftemp),
	   format_data(F_TIME, buf + 100, &wx.dew.outhi, ftd),
	   format_data(F_TEMP1, buf, &wx.dew.in, ftemp),
	   format_data(F_TEMP1, buf + 20, &wx.dew.inhi, ftemp),
	   format_data(F_TIME, buf + 40, &wx.dew.inhi, ftd),
	   format_data(F_TEMP1, buf + 160, &wx.dew.outlo, ftemp),
	   format_data(F_TIME, buf + 180, &wx.dew.outlo, ftd),
	   format_data(F_TEMP1, buf + 120, &wx.dew.inlo, ftemp),
	   format_data(F_TIME, buf + 140, &wx.dew.inlo, ftd));
    if (opt_a)
      printf(aformat,
	     format_val(F_DIFF1, buf + 220, wx.dew.outa.nlo, ftemp), "spread ",
	     offon[wx.dew.outa.on],
	     format_val(F_DIFF1, buf + 200, wx.dew.ina.nlo, ftemp), "spread ",
	     offon[wx.dew.ina.on]);

    printf("Wind Gust:%-6s@%-11s  Wind Gust Hi:%-6s@%-11s %s\n"
	   "      Avg:%-6s@%-11s    Wind Chill:%-6s   Lo:%-6s %s\n",
	   format_data(F_DIR, buf, &wx.wind.gdir, 0),
	   format_data(F_SPEED, buf + 20, &wx.wind.gspeed, fspeed),
	   format_data(F_DIR, buf + 40, &wx.wind.dirhi, 0),
	   format_data(F_SPEED, buf + 60, &wx.wind.speedhi, fspeed),
	   format_data(F_TIME, buf + 80, &wx.wind.speedhi, ftd),
	   format_data(F_DIR, buf + 100, &wx.wind.adir, 0),
	   format_data(F_SPEED, buf + 120, &wx.wind.aspeed, fspeed),
	   format_data(F_TEMP1, buf + 140, &wx.chill.temp, ftemp),
	   format_data(F_TEMP1, buf + 160, &wx.chill.low, ftemp),
	   format_data(F_TIME, buf + 180, &wx.chill.low, ftd));
    if (opt_a)
      printf("    Alarm:%-11s %s            Alarm:%-6s %s\n\n",
	     format_val(F_SPEED, buf + 200,  wx.wind.a.nhi, fspeed),
	     offon[wx.wind.a.on],
	     format_val(F_TEMP1, buf + 220,  wx.chill.a.nlo, ftemp),
	     offon[wx.chill.a.on]);

    printf("Rain Rate:%s  Yesterday:%s Total:%s since %s\n",
	   format_data(F_RATE, buf, &wx.rain.rate, fdepth),
	   format_data(F_DEPTH, buf + 20, &wx.rain.yest, fdepth),
	   format_data(F_DEPTH, buf + 40, &wx.rain.total, fdepth),
	   format_data(F_TIME, buf + 60, &wx.rain.total, ftd));
    if (opt_a)
      printf("    Alarm:%s %s\n\n",
	     format_val(F_RATE, buf + 80, wx.rain.a.nhi, fdepth),
	     offon[wx.rain.a.on]);

    printf("Barometer: %s at %s %s sea; 12-24hr forecast: %s\n",
	   trend[wx.baro.trend - 1],
	   format_data(F_PRESS, buf, &wx.baro.local, fbaro),
	   format_data(F_PRESS, buf + 20, &wx.baro.sea, fbaro),
	   pred[wx.baro.pred - 1]);
    if (opt_a) {
      printf("    Alarm:%s %s",
	     format_val(F_PRESS, buf + 40, wx.baro.a.nhi, fbaro),
	     offon[wx.rain.a.on]);
      if (!wx200ignoreunits)
	printf("      *=Out of Range, !=Sensor Error, #=Both * and !");
      printf("\n");
      fflush(stdout);
    }

  } while (opt_r && (num = wx200bufread(socket)) != EOF);

  if (query)
    printf("</pre>
<hr>
<address>
Weather by <a href=\"http://wx200d.sourceforge.net/\">wx200d</a>
version <b>%s</b>.
</address>
</body></html>
", VERSION);

  exit(wx200close(socket));
}
