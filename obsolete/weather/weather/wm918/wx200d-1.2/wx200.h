/*
 * @(#)$Id: wx200.h,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 1998 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file defines common WX200 data structures and function prototypes.
 *
 */

#define GROUPS	16		/* number of group buffers, indexes: */

#define GROUP0	0		/* reserved... */
#define GROUP1	1		/* WMR918 GROUP8 changed */
#define GROUP2	2		/* WMR918 GROUP9 changed */
#define GROUP3	3
#define GROUP4	4		/* WMR918 GROUPA changed */
#define GROUP5	5
#define GROUP6	6
#define GROUP7	7
#define GROUP8	8		/* Time, Humidity */
#define GROUP9	9		/* Temperature */
#define GROUPA	10		/* Barometer, Dew Point */
#define GROUPB	11		/* Rain */
#define GROUPC	12		/* Wind, Wind Chill */
#define GROUPD	13		/* (reserved place holder) */
#define GROUPE	14		/* (reserved place holder) */
#define GROUPF	15		/* Time */

#define GROUP_LENMAX	38	/* max buffer lenth, lengths: */
#define GROUP_LENGTHS	{0, 0, 0, 0, 0, 0, 0, 0, 35, 34, 31, 14, 27, 0, 0, 5}

extern int wx200open(char *);	/* client.c */
extern int wx200close(int);
extern char wx200host[], wx200error[];
extern int wx200port;

extern void wx200bufinit();	/* buffer.c */
extern int wx200bufread(int);
extern unsigned char groupbuf[GROUPS * 2][GROUP_LENMAX];
extern int grouplen[];

extern int wx200parse(int);	/* parse.c */
extern int wx200ignoreclock;	/* set to 1 to use localhost's clock instead */

extern int wx200stationtype;	/* serial.c */

typedef struct WX_SENS {	/* sensor or hi or lo memory */
  int val;			/* raw integer value of sensor or memory */
  float n;			/* base float unit value */
  int or;			/* out of range flag */
  int err;			/* sensor error flag, unused by memories */
  int sec;			/* second used only by the clock */
  int min;			/* time of memory, unused by sensors */
  int hour;
  int day;
  int mon;
} WX_SENS;
#define WX_HILO struct WX_SENS

typedef struct WX_ALRM {	/* alarm */
  int hi;
  float nhi;
  int lo;
  float nlo;
  int on;			/* on flag */
} WX_ALRM;

typedef struct WX_TIME {	/* Time */
  WX_SENS clock;
  WX_ALRM a;			/* hi=HH, lo=MM */
  int format;
} WX_TIME;

typedef struct WX_HUMID {	/* Humidity, n = percent */
  WX_SENS in;			/* 10<val<97 % @ 1 */
  WX_HILO inhi;
  WX_HILO inlo;
  WX_ALRM ina;
  WX_SENS out;			/* 10<val<97 % @ 1 */
  WX_HILO outhi;
  WX_HILO outlo;
  WX_ALRM outa;
} WX_HUMID;

typedef struct WX_TEMP {	/* Temperature, n = degrees Celsius */
  WX_SENS in;			/* 0<val<500 .1 degrees C @ 1 */
  WX_HILO inhi;
  WX_HILO inlo;
  WX_ALRM ina;			/* 32<lo/hi<122 degrees F @ 1 */
  WX_SENS out;			/* -40<val<600 .1 degrees C @ 1 */
  WX_HILO outhi;
  WX_HILO outlo;
  WX_ALRM outa;			/* -40<val<140 degrees F @ 1 */
  int format;
} WX_TEMP;

typedef struct WX_BARO {	/* Barometer, n = millibars */
  WX_SENS local;		/* 7950<val<10500 .1 mb @ 10 */
  WX_SENS sea;			/* 7950<val<10500 .1 mb @ 1 */
  int format;
  int pred;
  int trend;
  WX_ALRM a;			/* 1<hi<16 mb @ 1, lo unused */
} WX_BARO;

typedef struct WX_DEW {		/* Dew Point, n = degrees Celsius */
  WX_SENS in;			/* 0<val<47 degrees C @ 1 */
  WX_HILO inhi;
  WX_HILO inlo;
  WX_ALRM ina;			/* 1<lo<16 degrees C @ 1, hi unused */
  WX_SENS out;			/* 0<val<56 degrees C @ 1 */
  WX_HILO outhi;
  WX_HILO outlo;
  WX_ALRM outa;			/* 1<lo<16 degrees C @ 1, hi unused */
} WX_DEW;

typedef struct WX_RAIN {	/* Rain, n = mm/hr or mm */
  WX_SENS rate;			/* 0<val<998 mm/hr @ 1 */
  WX_HILO yest;			/* 0<val<9999 mm @ 1, timestamp not used */
  WX_HILO total;		/* 0<val<9999 mm @ 1 */
  int format;
  WX_ALRM a;			/* 0<hi<393 .1 in/hr @ 1, lo unused */
} WX_RAIN;

typedef struct WX_WIND {	/* Wind, n = meters per second or degrees */
  WX_SENS gspeed;		/* 0<val<560 .1 m/s @ 2 */
  WX_SENS gdir;			/* 0<val<359 compass degrees @ 1 */
  WX_SENS aspeed;
  WX_SENS adir;
  WX_HILO speedhi;
  WX_HILO dirhi;		/* timestamp not used, get it from speedhi */
  int format;
  WX_ALRM a;			/* 0<hi<56 m/s @ 1, lo unused */
} WX_WIND;

typedef struct WX_CHILL {	/* Wind Chill, n = degrees Celsius */
  WX_SENS temp;			/* -85<val<60 degrees C @ 1 */
  WX_HILO low;
  WX_ALRM a;			/* -121<lo<140 degrees F @ 1, hi unused */
} WX_CHILL;

typedef struct WX_GEN {
  int power;			/* 0=AC, 1=DC */
  int lowbat;			/* 0=good, 1=low */
  int section;			/* 0<section<7 */
  int screen;			/* 0<screen<3 */
  int subscreen;		/* 0<subscreen<3 */
} WX_GEN;

typedef struct WX_UNK {		/* bytes that may contain some unknown bits */
  unsigned char eightf5;
  unsigned char eightf32;
  unsigned char eightf33;
  unsigned char ninef13;
  unsigned char ninef15;
  unsigned char ninef28;
  unsigned char ninef30;
  unsigned char ninef31;
  unsigned char af5;
  unsigned char af6;
  unsigned char af28;
  unsigned char af29;
  unsigned char bf2;
  unsigned char bf10;
  unsigned char bf12;
  unsigned char cf14;
  unsigned char cf15;
  unsigned char cf21;
  unsigned char cf23;
  unsigned char cf24;
  unsigned char cf25;
} WX_UNK;

typedef struct WX {		/* all data in the weather station object */
  WX_TIME td;
  WX_HUMID humid;
  WX_TEMP temp;
  WX_BARO baro;
  WX_DEW dew;
  WX_RAIN rain;
  WX_WIND wind;
  WX_CHILL chill;
  WX_GEN gen;
  WX_UNK unk;
} WX;
extern WX wx;

#define TABLEN	256		/* tab.c */
extern char tabbuf[TABLEN];
extern void wx200tabinit();
extern int wx200tab(int);

extern float unit_humid(float, int); /* unit.c */
extern float unit_temp(float, int);
extern float unit_press(float, int);
extern float unit_depth(float, int);
extern float unit_speed(float, int);
extern int unit_dir(float, int);

#define F_HUMID	0		/* format.c */
#define F_TEMP	1		/* tenths for temp */
#define F_TEMP1	2		/* units for dew point and wind chill */
#define F_DIFF1	3		/* temperature diff for dew point alarm */
#define F_PRESS	4
#define F_DEPTH	5
#define F_RATE	6
#define F_SPEED	7
#define F_DIR	8
#define F_TIME	9

extern int wx200ignoreflags;	/* set to 1 to suppress flag output (*!#) */
extern int wx200ignoreunits;	/* set to 1 to suppress unit output */

typedef struct WX_UNIT {
  int w;			/* width */
  int p;			/* precision */
  char *l;			/* wx200 display label */
  char *d;			/* text description */
} WX_UNIT;

typedef struct WX_FORMAT {	/* all unit formats */
  WX_UNIT humid[2];
  WX_UNIT temp[2];
  WX_UNIT temp1[3];
  WX_UNIT press[4];
  WX_UNIT depth[2];
  WX_UNIT rate[2];
  WX_UNIT speed[4];
  char *wind4[4];
  char *wind8[8];
  char *wind16[16];
} WX_FORMAT;
extern WX_FORMAT wxformat;

extern char *format_data(int, char *, WX_SENS *, int);
extern char *format_val(int, char *, float, int);
