/*

   wx2000 -- Weather Data Logger Extration Program (ws2000 model)

   Copyright (C) 2000 Friedrich Zabel

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   $Id: wx2000.h,v 1.1 2002/09/05 21:39:29 jku Exp $
*/

#define SOH 0x01
#define STX 0x02
#define ETX 0x03
#define EOT 0x04
#define ENQ 0x05
#define ACK 0x06
#define NAK 0x15
#define DLE 0x10
#define DC2 0x12
#define DC3 0x13

#define FALSE 0
#define TRUE 1

#define BUFLEN 255

#define TIOCMGET	0x5415
#define TIOCMSET	0x5418
#define TIOCM_DTR	0x002


volatile int STOP=FALSE;
volatile int CONT=TRUE;
void signal_handler_IO (int status);   /* definition of signal handler */
int wait_flag=TRUE;                    /* TRUE while no signal received */
u_char final_data[BUFLEN];

u_char write_data[4];
u_char interval_min;

/* data structures */

#pragma pack(1)			/* all folowing data is byte aligned */

typedef struct {
	unsigned char new;	/* new = 0 DATA is OLD (No Reception) , new = 1 DATA is NEW (Reception OK) */
	float temp;		/* temperature in Celcius */
	unsigned char hum;	/* humidity in % */
	unsigned char flag;	/* ... */
} WX2000_TEMP_HUM;		/* Structure fot Temperature & Humidity Sensor */

typedef struct {
	unsigned char new;	/* new = 0 DATA is OLD (No Reception) , new = 1 DATA is NEW (Reception OK) */
	float pressure;		/* pressure in hPa */
	float temp;		/* temperature in Celcius */
	unsigned char hum;	/* humidity in % */
	unsigned char flag;
} WX2000_TEMP_HUM_PRESSURE;	/* Structure for Temperature / Humidity & Pressure Sensor */

typedef struct {
	unsigned char new;	/* new = 0 DATA is OLD (No Reception) , new = 1 DATA is NEW (Reception OK) */
	float pressure;		/* pressure in hPa */
	float temp;		/* temperature in Celcius */
	unsigned char hum;	/* humidity in % */
	unsigned char flag;
} WX2000_TEMP_HUM_PRESSURE_INSIDE; /* Structure for Inside Temperature / Humidity & Pressure Sensor */

typedef struct {
	unsigned char new;	/* new = 0 DATA is OLD (No Reception) , new = 1 DATA is NEW (Reception OK) */
	float speed;		/* Speed in KM/h */
	short angle;		/* 0 = North , 180 = South */
	unsigned char width;	/* +- degrees from angle */
	unsigned char flag;
} WX2000_WIND;			/* Structure for WindSensor */

typedef struct {
	unsigned char new;	/* new = 0 DATA is OLD (No Reception) , new = 1 DATA is NEW (Reception OK) */
	short counter;		/* each Counter +- = 360ml */
	unsigned char flag;
} WX2000_RAIN;			/* Structure for RainSensor */

typedef struct {
	short errors;
	unsigned char flag;
} SENSOR;

typedef struct {
	short interval;
	unsigned char dcf;
	unsigned char hf;
	unsigned char sensors;
	unsigned char dcf_sync;
	unsigned char low_bat;
	float version_no;
} WX2000_INFO;

typedef struct {
	unsigned char dcf_status;
	short weekday;
	short hour;
	short min;
	short sec;
	short day;
	short month;
	short year;		/* +- Fix for Y2K problem */
} DCF_dt;

#pragma pack()			/* all following data is 4byte aligned */

typedef struct {
	int blocknr;
	time_t time;
	WX2000_TEMP_HUM t_h[8]; /* 9 Sensors */
	WX2000_TEMP_HUM_PRESSURE t_h_p[5]; /* 6 Sensors */
	WX2000_TEMP_HUM_PRESSURE_INSIDE thp_i; /* 1 Sensor */
	WX2000_WIND wind;
	WX2000_RAIN rain;
} WX2000_DATA;
