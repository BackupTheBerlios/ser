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

   $Id: wx2000.c,v 1.1 2002/09/05 21:39:29 jku Exp $
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <time.h>

#include "config.h"
#include "wx2000.h"
#include "lock_file.c"
#include "serial.c"

WX2000_DATA data;
SENSOR sensor[17];  /* 18 Sensors */
WX2000_INFO wx2000_info;
DCF_dt DCF_date_time;
struct tm *data_time;
int loop_count=0;
int fileout;

/* ------------------------------------------------------------------------------ */

int check_error(void) {
 	 if(final_data[0] == STX)
	  if(final_data[1] == 0x01)
	   if(final_data[2] == NAK)
	    if(final_data[3] == 0xE8)
	     if(final_data[4] == ETX) {
#ifdef DEBUG
		printf(" ERROR NAK !!!!!!!! \n");
#endif DEBUG
		return -1;
		}
	else
		return 0;
} /* END check_error() */

/* ------------------------------------------------------------------------------ */

int check_ack(void) {
	 if(final_data[0] == STX)
	  if(final_data[1] == 0x01)
	   if(final_data[2] == ACK)
	    if(final_data[3] == 0xF7)
	     if(final_data[4] == ETX) {
#ifdef DEBUG
	    	printf("OK ACK Acepted\n");
#endif DEBUG
	    	return 0;
	    	}
	else
		return -1;
} /* END check_ack() */

/* ------------------------------------------------------------------------------ */

int check_dle(void) {
	 if(final_data[0] == STX)
	  if(final_data[1] == 0x01)
	   if(final_data[2] == DLE)
	    if(final_data[3] == 0xED)
	     if(final_data[4] == ETX) {
#ifdef DEBUG
	    	printf("OK DLE Acepted\n");
#endif DEBUG
	    	return 0;
	    	}
	else
		return -1;
} /* END check_dle() */

/* ------------------------------------------------------------------------------ */

int clean_data(int data_l) { 

	int a=0,temp=0;
#ifdef DEBUG
	int z=0;
#endif DEBUG
	
	for(a=0;a<data_l;a++) {
#ifdef DEBUG
		printf("%d,%d ",a,temp);
#endif DEBUG
		if(final_data[a] == ENQ && final_data[a+1] == DC2) {
			final_data[ a - (a - temp) ] = STX;
			a++;
			}
		else
		if(final_data[a] == ENQ && final_data[a+1] == DC3) {
			final_data[ a - (a - temp) ] = ETX;
			a++;
			}
		else
		if(final_data[a] == ENQ && final_data[a+1] == NAK) {
			final_data[ a - (a - temp) ] = ENQ;
			a++;
			}
		else
		  final_data[temp] = final_data[a];
	temp++;
#ifdef DEBUG
       	for(z=0;z<data_l;z++) {
	printf("%x ",final_data[z]);
	}
	printf("\n");
#endif DEBUG	
	}
#ifdef DEBUG
       	for(a=0;a<data_l;a++) {
	printf("%x ",final_data[a]);
	}
	printf("\n");
#endif DEBUG
return temp;
} /* END clean_data() */

/* ------------------------------------------------------------------------------ */

int verify_checksum(int data_l) {

	int a;
	u_char sum;
	
	for(a=0;a<data_l-1;a++) {
		sum += final_data[a];
		}
#ifdef DEBUG
	printf("\nsum : %x\n",sum);
#endif DEBUG
	sum &= 0xff;
#ifdef DEBUG
	printf("sum : %x\n",sum);
#endif DEBUG
	return sum;
} /* END verify_checksum() */

/* ------------------------------------------------------------------------------ */

int verify_length(u_char length) {

	if(final_data[length+3] == ETX)
		return 0;
	else
		return -1;
} /* END verify_length() */

/* ------------------------------------------------------------------------------ */

calculate_dcf(int data_length) {

	int a,data_l,tmp_year;
	u_char length;

	data_l = clean_data(data_length);
	verify_checksum(data_l);
	length = final_data[1];
	verify_length(length);
#ifdef DEBUG
	printf("Length : %d\n",length);
#endif DEBUG
	/* REMOVE <STX> & LENGTH */
	for(a=0;a<(data_length-1);a++) {
		final_data[a] = final_data[a+2];
		}
#ifdef DEBUG
       	for(a=0;a<data_l-2;a++) {
	printf("%x ",final_data[a]);
        }
	printf("\n");
#endif DEBUG	

	DCF_date_time.dcf_status = (final_data[0]>>4&0x01);
	DCF_date_time.weekday = (final_data[0]&0x07);
	DCF_date_time.hour = (final_data[1]>>4&0x0f)*10+(final_data[1]&0x0f);
	DCF_date_time.min = (final_data[2]>>4&0x0f)*10+(final_data[2]&0x0f);
	DCF_date_time.sec = final_data[3]&0xff;
	DCF_date_time.day = (final_data[4]>>4&0x0f)*10+(final_data[4]&0x0f);
	DCF_date_time.month = (final_data[5]>>4&0x0f)*10+(final_data[5]&0x0f);
	tmp_year = (final_data[6]>>4&0x0f)*10+(final_data[6]&0x0f);
	if(tmp_year >= 99) 	/* for Y2K problem correction - bad programming ;) */
		DCF_date_time.year = tmp_year + 1900;
	else 
		if(tmp_year <= 38)	/* 2038 hmmm will this software still be in use then ;) */
			DCF_date_time.year  = tmp_year + 2000;
	printf("DCF OK:  %d\n",DCF_date_time.dcf_status);
	printf("Weekday: %d\n",DCF_date_time.weekday);
	printf("Time:    %02d:%02d:%02d\n",DCF_date_time.hour,DCF_date_time.min,DCF_date_time.sec);
	printf("Day:     %02d\n",DCF_date_time.day);
	printf("Month:   %02d\n",DCF_date_time.month);
	printf("Year:    %02d\n",DCF_date_time.year);

} /* END calculate_dcf() */

/* ------------------------------------------------------------------------------ */

calculate_data(int data_length) {

int a,b,data_l;
u_char length;

char write_buf[BUFLEN];

/* REMOVE <STX> & LENGTH */
	data_l = clean_data(data_length);
	length = final_data[1];
#ifdef DEBUG
       	for(a=0;a<data_l-6;a++) {
	printf("%x ",final_data[a]);
        }
	printf("Length : %d\n",length);
#endif DEBUG
	data.blocknr = ((final_data[2]&0xff)+(final_data[3]<<8&0xff00));
// 	printf("Block Nr: %d\n", data.blocknr);
// 	printf("Block Time: %d Min\n",(final_data[4]&0xff)+(final_data[5]<<8&0xff00));
	data.time = time(NULL) - (((final_data[4]&0xff)+(final_data[5]<<8&0xff00))*60);
	data_time = localtime(&data.time);
//	printf("Zeit : %d:%d:%d\n",data_time->tm_hour,data_time->tm_min,data_time->tm_sec);
//	printf("Date : %d/%d/%d\n",data_time->tm_mday,data_time->tm_mon+1,data_time->tm_year);
// 	printf("Block Mess Zeit : %d:%d:%d\n",
	for(a=0;a<(data_length - 5);a++) {
		final_data[a] = final_data[a+6];
	}
#ifdef DEBUG
       	for(a=0;a<data_l-6;a++) {
	printf("%x ",final_data[a]);
        }
	printf("\n");
#endif DEBUG

/* Begin External Temperature / Humidity Sensors 1, 3, 5 & 7 */
	for(a=0;a<8;a=a+2) {	/* For temp & humidity sensor 1,3,5,7 */
		b=a+(3*(a/2));
		if(final_data[b+1]&0x08 == 8) /* negative or positive temp */
			data.t_h[a].flag = 1; /* positive */
		else
			data.t_h[a].flag = 0; /* negative */
		data.t_h[a].temp = ((final_data[b+1]&0x07)*10 + (final_data[b]>>4&0x0f) + ((final_data[b]&0x0f) / (float) 10));
		data.t_h[a].hum = (final_data[b+1]>>4&0x0f) + (final_data[b+2]<<4&0x70);
		if((final_data[b+2]&0x08) == 8) {
			data.t_h[a].new = 1;
			}
		else
			data.t_h[a].new = 0;
	}
/* END External Temperature / Humidity Sensors 1, 3, 5 & 7 */

/* Begin External Temperature / Humidity Sensors 2, 4 & 6 */
	for(a=1;a<6;a=a+2) { /* For temp & humidity sensor 2,4,6 */
		b=a+(3*((a-1)/2))+1;
		if(final_data[b+1]>>4&0x08 == 8) /* negative or positive temp */
			data.t_h[a].flag = 1; /* positive */
		else
			data.t_h[a].flag = 0; /* negative */
		data.t_h[a].temp = ((final_data[b+1]>>4&0x07)*10 + (final_data[b+1]&0x0f) + ((final_data[b]>>4&0x0f) / (float) 10));
		data.t_h[a].hum = (final_data[b+2]&0x7f);
		if((final_data[b+2]>>4&0x08) == 8) {
			data.t_h[a].new = 1;
			}
		else
			data.t_h[a].new = 0;
	}
/* END External Temperature / Humidity Sensors 2, 4 & 6 */

/* Begin Internal Temperature / Humidity / Pressure Sensor */
	if(final_data[28]&0x08 == 8) /* negative or positive temp */
		data.thp_i.flag = 1; /* positive */
	else
		data.thp_i.flag = 0; /* negative */
	data.thp_i.temp = ((final_data[28]&0x07)*10 + (final_data[27]>>4&0x0f) + ((final_data[27]&0x0f) / (float) 10));
	data.thp_i.hum = (final_data[28]>>4&0x0f) + (final_data[29]<<4&0x70);
	data.thp_i.pressure = ((final_data[26]>>4&0x0f)*100) + ((final_data[26]&0x0f)*10) + (final_data[25]>>4&0x0f) + (ABOVE_NN / (float) 8) + 200;
	if((final_data[29]&0x08) == 8) {
		data.thp_i.new = 1;
		}
	else
		data.thp_i.new = 0;
/* END Internal Temperature / Humidity / Pressure Sensor */

/* Begin Wind Sensor */
	data.wind.speed = (final_data[23]>>4&0x07)*100 + (final_data[23]&0x0f)*10 + (final_data[22]>>4&0x0f) + ((final_data[22]&0x0f) / (float) 10);
	data.wind.angle = ((final_data[25]&0x03)*100) + ((final_data[24]>>4&0x0f)*10) + (final_data[24]&0x0f);
	if((final_data[23]>>4&0x08) == 8) {
		data.wind.new = 1;
		}
	else
		data.wind.new = 0;
/* END Wind Sensor

/* Begin Rain Sensor */

	data.rain.counter = (final_data[20])+(final_data[21]<<8&0x0f00);
	if((final_data[21]>>4&0x08) == 8) {
		data.rain.new = 1;
		}
	else
		data.rain.new = 0;
/* END Rain Sensor */

if(length == 60) {	/* if 16 sensors activated */
/* Begin Sensor 9 */
	if(final_data[30]>>4&0x08 == 8) /* negative or positive temp */
		data.t_h[8].flag = 1; /* positive */
	else
		data.t_h[8].flag = 0; /* negative */
		data.t_h[8].temp = ((final_data[30]>>4&0x07)*10 + (final_data[30]&0x0f) + ((final_data[29]>>4&0x0f) / (float) 10));
		data.t_h[8].hum = (final_data[31]&0x7f);
	if((final_data[31]>>4&0x08) == 8) {
		data.t_h[8].new = 1;
		}
	else
		data.t_h[8].new = 0;
/* END Sensor 9 */

/* Begin External Humidity / Temperature / Pressure Sensors 10, 11, 12, 13, 14 & 15 */
	for(a=0;a<7;a++) {
		b=32+(4*a);
		data.t_h_p[a].pressure = ((final_data[b+1]&0x0f)*100) + ((final_data[b]>>4&0x0f)*10) + (final_data[b]&0x0f) + (ABOVE_NN / (float) 8) + 200;
		if(final_data[b+2]>>4&0x08 == 8) /* negative or positive temp */
			data.t_h_p[a].flag = 1; /* positive */
		else
			data.t_h_p[a].flag = 0; /* negative */
		data.t_h_p[a].temp = ((final_data[b+2]>>4&0x07)*10 + (final_data[b+2]&0x0f) + ((final_data[b+1]>>4&0x0f) / (float) 10));
		data.t_h_p[a].hum = (final_data[b+3]&0x7f);
		if((final_data[b+3]>>4&0x08) == 8) {
			data.t_h_p[a].new = 1;
			}
		else
			data.t_h_p[a].new = 0;
	}
/* END External Humidity / Temperature / Pressure Sensors */
} /* END if(length) */

#ifdef DEBUG
	printf("Temp-Out:      %2.1f C\n",data.t_h[0].temp);
	printf("Hum-Out:       %d\n",data.t_h[0].hum);
	printf("NEW(OK) - Out: %d\n",data.t_h[0].new);
	printf("Temp-In:       %2.1f C\n",data.thp_i.temp);
	printf("Hum-In:        %d\n",data.thp_i.hum);
	printf("Pressure:      %.0f hPa\n",data.thp_i.pressure);
	printf("NEW(OK) - In : %d\n",data.thp_i.new);
	printf("WindSpeed:     %3.1f Km/h\n",data.wind.speed);
	printf("WindSpeed:     %3.2f m/s\n",data.wind.speed / (float) 3600 * (float) 1000);
	printf("WindDirection: %d\n",data.wind.angle);
	printf("NEW(OK) - Wind:%d\n",data.wind.new);
	printf("Rain Counter:  %d  Rain Calculated: %.1f mm\n",data.rain.counter,data.rain.counter * 360 / (float) 1000);
	printf("NEW(OK) - Rain:%d\n",data.rain.new);
#endif DEBUG

/* BEGIN Print data line */
	printf("%04d ",data.blocknr);
	printf("%02d:%02d:%02d ",data_time->tm_hour,data_time->tm_min,data_time->tm_sec);

	if(data_time->tm_year >= 99) 	/* for Y2K problem correction - bad programming ;) */
		data_time->tm_year += 1900;
	else 
		if(data_time->tm_year <= 38)	/* 2038 hmmm will this software still be in use then ;) */
			data_time->tm_year += 2000;

	printf("%02d/%02d/%04d ",data_time->tm_mday,data_time->tm_mon+1,data_time->tm_year);
	if(data.t_h[0].flag = 0) printf("-");
	printf("%2.1f %d ",data.t_h[0].temp,data.t_h[0].hum);
	printf("%d ",data.t_h[0].new);
	if(data.thp_i.flag = 0) printf("-");
	printf("%2.1f %d %.0f ",data.thp_i.temp,data.thp_i.hum,data.thp_i.pressure);
	printf("%d ",data.thp_i.new);
	printf("%3.1f %d ",data.wind.speed,data.wind.angle);
	printf("%d ",data.wind.new);
	printf("%.1f ",data.rain.counter * 360 / (float) 1000);
	printf("%d\n",data.rain.new);
/* END Print data line */

/* open datafile datafile.dat and write all data */
	
/* 	fileout = open("./datafile.dat", O_WRONLY|O_CREAT );
	if (fileout < 0) {
		printf("Error with file\n");
		exit(-1);
	}
	sprintf(write_buf, "%04d \n",data.blocknr);
	sprintf(write_buf, "%02d:%02d:%02d ",data_time->tm_hour,data_time->tm_min,data_time->tm_sec);
	sprintf(write_buf, "%02d/%02d/%04d ",data_time->tm_mday,data_time->tm_mon+1,data_time->tm_year);
	if(data.t_h[0].flag = 0) sprintf(write_buf, "-");
	sprintf(write_buf, "%2.1f %d ",data.t_h[0].temp,data.t_h[0].hum);
	sprintf(write_buf, "%d ",data.t_h[0].new);
	if(data.thp_i.flag = 0) sprintf(write_buf, "-");
	sprintf(write_buf, "%2.1f %d %.0f ",data.thp_i.temp,data.thp_i.hum,data.thp_i.pressure);
	sprintf(write_buf, "%d ",data.thp_i.new);
	sprintf(write_buf, "%3.1f %d ",data.wind.speed,data.wind.angle);
	sprintf(write_buf, "%d ",data.wind.new);
	sprintf(write_buf, "%.1f ",data.rain.counter * 360 / (float) 1000);
	sprintf(write_buf, "%d\n",data.rain.new);

	write(fileout, write_buf, strlen(write_buf));
*/
	fflush(stdout);
//	tcflush(fileout, TCIFLUSH);
} /* END calculate_data() */

/* ------------------------------------------------------------------------------ */

show_status(int data_length) {

int a,data_l;
u_char length;

	data_l = clean_data(data_length);
	length = final_data[1];
#ifdef DEBUG
	printf("Length : %d\n",length);
#endif DEBUG
	/* REMOVE <STX> & LENGTH */
	for(a=0;a<(data_length - 1);a++) {
		final_data[a] = final_data[a+2];
	}
#ifdef DEBUG
       	for(a=0;a<data_l-2;a++) {
		printf("%x ",final_data[a]);
        }
	printf("\n");
#endif DEBUG

	wx2000_info.interval = final_data[18]&0xff;
	wx2000_info.dcf = final_data[19]&0x01;
	wx2000_info.hf = final_data[19]>>1&0x01;
	wx2000_info.sensors = final_data[19]>>2&0x01;
	wx2000_info.dcf_sync = final_data[19]>>3&0x01;
	wx2000_info.low_bat = final_data[19]>>4&0x01;
	wx2000_info.version_no = (final_data[20]>>4&0x0f) + ((final_data[20]&0x0f) / (float) 10);
	for(a=0;a<=18;a++) {
		if((final_data[a]&0xff) >= 16) {
			sensor[a].flag = 1;
			if((final_data[a]&0xff) > 16) {
				sensor[a].errors = (final_data[a]&0xff) - 16;
				}
			else
				sensor[a].errors = 0;
			}
		else
			sensor[a].flag = 0;
	}
if(sensor[0].flag)
printf("Temp/Humidity 1 :  %d ReceiveErrors : %d\n",sensor[0].flag,sensor[0].errors);
if(sensor[1].flag)
printf("Temp/Humidity 2 :  %d ReceiveErrors : %d\n",sensor[1].flag,sensor[1].errors);
if(sensor[2].flag)
printf("Temp/Humidity 3 :  %d ReceiveErrors : %d\n",sensor[2].flag,sensor[2].errors);
if(sensor[3].flag)
printf("Temp/Humidity 4 :  %d ReceiveErrors : %d\n",sensor[3].flag,sensor[3].errors);
if(sensor[4].flag)
printf("Temp/Humidity 5 :  %d ReceiveErrors : %d\n",sensor[4].flag,sensor[4].errors);
if(sensor[5].flag)
printf("Temp/Humidity 6 :  %d ReceiveErrors : %d\n",sensor[5].flag,sensor[5].errors);
if(sensor[6].flag)
printf("Temp/Humidity 7 :  %d ReceiveErrors : %d\n",sensor[6].flag,sensor[6].errors);
if(sensor[7].flag)
printf("Temp/Humidity 8 :  %d ReceiveErrors : %d\n",sensor[7].flag,sensor[7].errors);
if(sensor[8].flag)
printf("Rain :             %d ReceiveErrors : %d\n",sensor[8].flag,sensor[8].errors);
if(sensor[9].flag)
printf("Wind :             %d ReceiveErrors : %d\n",sensor[9].flag,sensor[9].errors);
if(sensor[10].flag)
printf("Internal :         %d ReceiveErrors : %d\n",sensor[10].flag,sensor[10].errors);
if(sensor[11].flag)
printf("Temp/Humidity 9 :  %d ReceiveErrors : %d\n",sensor[11].flag,sensor[11].errors);
if(sensor[12].flag)
printf("Temp/Humidity 10 : %d ReceiveErrors : %d\n",sensor[12].flag,sensor[12].errors);
if(sensor[13].flag)
printf("Temp/Humidity 11 : %d ReceiveErrors : %d\n",sensor[13].flag,sensor[13].errors);
if(sensor[14].flag)
printf("Temp/Humidity 12 : %d ReceiveErrors : %d\n",sensor[14].flag,sensor[14].errors);
if(sensor[15].flag)
printf("Temp/Humidity 13 : %d ReceiveErrors : %d\n",sensor[15].flag,sensor[15].errors);
if(sensor[16].flag)
printf("Temp/Humidity 14 : %d ReceiveErrors : %d\n",sensor[16].flag,sensor[16].errors);
if(sensor[17].flag)
printf("Temp/Humidity 15 : %d ReceiveErrors : %d\n",sensor[17].flag,sensor[17].errors);
printf("Interval :         %d\n",wx2000_info.interval);
printf("DCF :              %d\n",wx2000_info.dcf);
printf("HF :               %d\n",wx2000_info.hf);
if(wx2000_info.sensors==0)
	printf("Sensors :          8\n");
else
	printf("Sensors	:          16\n");
printf("DCF sync :         %d\n",wx2000_info.dcf_sync);
printf("Low Bat :          %d\n",wx2000_info.low_bat);
printf("Version :          %1.1f\n",wx2000_info.version_no);

} /* END show_status() */

/* ------------------------------------------------------------------------------ */

int get_data(int c,int fd) {

int 	a,
	res,
	blip=0,
	buf_count=0,
	stat=0;

long	internal_loop=0;


u_char fetch_data[255];

unsigned int arg;

for(a=0;a<BUFLEN;a++) {
	final_data[a] = 0;
	}

buf_count = 0;
internal_loop = 0;

STOP=FALSE;
wait_flag=TRUE;

if(loop_count>0) {
	if(c == 110) {
#ifdef DEBUG
		printf("110-test l:%d\n",loop_count);
#endif DEBUG
		write_data[0] = SOH;
		write_data[1] = 0x32;
		write_data[2] = 256 - (write_data[0] + write_data[1]);
		write_data[3] = EOT;
		write(fd, write_data, 4); /* Next data block */
#ifdef DEBUG
		for(a=0;a<255;a++) {
			printf("%x ",final_data[a]);
		}
#endif DEBUG
	}
	if(c == 103) {
#ifdef DEBUG
		printf("103-test l:%d\n",loop_count);
#endif DEBUG
		write_data[0] = SOH;
		write_data[1] = 0x31;
		write_data[2] = 256 - (write_data[0] + write_data[1]);
		write_data[3] = EOT;
		write(fd, write_data, 4); /* Read Data Block */
	}
}

while (STOP==FALSE) {

if(internal_loop>100000000) STOP=TRUE;	/* Some Timeout Value to exit if no response (change to check seconds) */
internal_loop++;
if (wait_flag==FALSE) {
	internal_loop = 0; 
	res = read(fd,fetch_data,255);
	for(a=0;a<res;a++) {
		final_data[buf_count+a] = fetch_data[a];
		}
#ifdef DEBUG
	printf("bufc : %d\n",buf_count);
#endif DEBUG
	buf_count += res;		
	if (res > 0) {

	if ((fetch_data[0] == ETX) && (loop_count == 0)) {

		fetch_data[0] = 0;
		final_data[0] = 0;
		buf_count -= res;
		res--;

		switch(c) {
		case 'd' :
			write_data[0] = SOH;
			write_data[1] = 0x30;
			write_data[2] = 256 - (write_data[0] + write_data[1]);
			write_data[3] = EOT;
			write(fd, write_data, 4); /* DCF */
			break;
		case 'g' :
#ifdef DEBUG
			printf("103-test-0loop\n");
#endif DEBUG
			write_data[0] = SOH;
			write_data[1] = 0x31;
			write_data[2] = 256 - (write_data[0] + write_data[1]);
			write_data[3] = EOT;
			write(fd, write_data, 4); /* Read Data Block */
			break;
		case 'n' :
#ifdef DEBUG
			printf("TEST - NEXT\n");
#endif DEBUG
			write_data[0] = SOH;
			write_data[1] = 0x32;
			write_data[2] = 256 - (write_data[0] + write_data[1]);
			write_data[3] = EOT;
			write(fd, write_data, 4); /* Next data block */
			break;
		case '9' :
			write_data[0] = SOH;
			write_data[1] = 0x33;
			write_data[2] = 256 - (write_data[0] + write_data[1]);
			write_data[3] = EOT;
			write(fd, write_data, 4); /* 9 Sensoren Setzen */
			break;
		case 'F' :
			write_data[0] = SOH;
			write_data[1] = 0x34;
			write_data[2] = 256 - (write_data[0] + write_data[1]);
			write_data[3] = EOT;
			write(fd, write_data, 4); /* 16 Sensoren Setzen */
			break;
		case 'i' :
			write_data[0] = SOH;
			write_data[1] = 0x36;
			interval_min = 5;
			write_data[2] = interval_min;
			write_data[3] = 256 - (write_data[0] + write_data[1] + write_data[2]);
			write_data[4] = EOT;
			write(fd, write_data, 5); /* Set Interval in Min */
			break;
		case 's' :
			write_data[0] = SOH;
			write_data[1] = 0x35;
			write_data[2] = 256 - (write_data[0] + write_data[1]);
			write_data[3] = EOT;
			write(fd, write_data, 4); /* Status */
			break;
		}
#ifdef DEBUG
		printf ("ETX !! \n");
#endif DEBUG
	}
        else if (res==1) STOP=TRUE; /* stop loop if only 1 CHAR was input except ETX */

#ifdef DEBUG
//	printf(":%s:%d\n", fetch_data, res);
	for(a=0;a<res;a++) {
        	printf("%x ",fetch_data[a]);
      }
#endif DEBUG
          wait_flag = TRUE;      /* wait for new input */
          } /* END if(res>0) */


/* Test For End Of Data & then Process--------------- */

 	if (res > 1 && final_data[buf_count - 1] == ETX) {
		STOP = TRUE;
#ifdef DEBUG
		printf ("END\n");
		printf("bufx :%dx",buf_count);
	       	for(a=0;a<buf_count;a++) {
       		printf("%x ",final_data[a]);
        		}
		printf("\n");
#endif DEBUG
		if (check_error()<0) {
			printf("ERROR\n");
			exit(-1);
			}
		switch(c) {
		case 'd' :
			calculate_dcf(buf_count);
			stat = 0;
			break;
		case 'g' :
			stat = check_dle();
			if(stat == 0) {
				printf("NO more blocks\n");
				stat = -1;
			}
			else {
			calculate_data(buf_count);
			stat = 0;
			}
			break;
		case 'n' :
       			stat = check_ack();
			if(stat == 0) {
#ifdef DEBUG
				printf("OK next block selected\n");
#endif DEBUG
				stat = 0;
			}
			else {
				stat = check_dle();
				if(stat == 0) {
#ifdef DEBUG
					printf("NO more blocks\n");
#endif DEBUG
					stat = -1;
				}
			}
			if(stat == -1) {
#ifdef DEBUG
				printf("Error occured\n");
#endif DEBUG
				stat = -1;
			}
			break;
		case '9' :
			stat = check_ack();
			break;
		case 'F' :
			stat = check_ack();
			break;
		case 's' :
			show_status(buf_count);
			stat = 0;
			break;
		}		
#ifdef DEBUG
 		printf("\n");
#endif DEBUG
		}

	 } /* END if */
	} /* END while */
loop_count++;
return stat;
} /* END get_data() */

/* ------------------------------------------------------------------------------ */

int main(int argc, char *argv[]) {

int c,info,fd;
u_char tmp[1];

const char *flags = "dgn9Fisx";

if(argc<2) {
	fprintf(stderr, "wx2000 v0.3, Copyright (C) 1999-2000 Friedrich Zabel (fredz@mail.telepac.pt)\n");
	fprintf(stderr, "wx2000 comes with ABSOLUTELY NO WARRANTY;This is free software,\n");
	fprintf(stderr, "and you are welcome to redistribute it under certain conditions;\n");
	fprintf(stderr, "see file COPYING for details\n");
	fprintf(stderr, "\nUsage:\n");
	fprintf(stderr, "wx2000 -d - Get DCF Status & Info\n");
	fprintf(stderr, "wx2000 -g - Fetch 1 DataBlock (Good for Testing, doesn't delete)\n");
	fprintf(stderr, "wx2000 -n - Select 2 DataBlocks Ahead (!! For Testing ONLY !!)\n");
	fprintf(stderr, "wx2000 -9 - Set 9 Sensors (!! Clears All Data for Logger !!)\n");
	fprintf(stderr, "wx2000 -F - Set 16 Sensors (!! Clears All Data for Logger !!)\n");
//	fprintf(stderr, "wx2000 -i MIN - Set Logging Interval in MIN (2-60)\n");
	fprintf(stderr, "wx2000 -s - Get Logger Status & Info\n");
	fprintf(stderr, "wx2000 -x - Fetch All Data Until No More, do > datafile\n");
	exit(1);
	}

fd = open_port();
setserial(fd);

c = getopt(argc, argv, flags);
set_dtr(fd);
switch(c) {
	case 'd' :
		info = get_data(c,fd);
		break;
	case 'g' :
		info = get_data(c,fd);
		break;
	case 'n' :
		info = get_data(c,fd);
		break;			
	case '9' :
		info = get_data(c,fd);
		break;
	case 'F' :
		info = get_data(c,fd);
		break;
	case 'i' :
#ifdef DEBUG
		printf("%s", argv[2]);
#endif DEBUG
		strncpy(tmp, argv[2], 2);
#ifdef DEBUG
		printf("Interval Argv : %x %x\n",tmp[0], tmp[1]);
#endif DEBUG
		if(tmp[1]>0)
			interval_min = (((tmp[0] - 48)*10) + (tmp[1] - 48));
		else
			interval_min = (tmp[0] - 48);
#ifdef DEBUG
		printf("intv : %x\n",interval_min);
#endif DEBUG		
		if((interval_min < 2) || (interval_min > 60)) {
			printf("Interval Must be between 2 - 60 Min\n");
			exit(-1);
		}
//		info = get_data(c,fd);
		break;
	case 's' :
		info = get_data(c,fd);
		break;
	case 'x' :
		while (CONT == TRUE) {
			info = get_data('g',fd);
#ifdef DEBUG
			printf("info1:%d\n",info);
#endif DEBUG
			if(info < 0)
				CONT = FALSE;
			else if(info == 0) {
				info = get_data('n',fd);
#ifdef DEBUG
				printf("info2:%d\n",info);
#endif DEBUG
				if(info < 0)
					CONT = FALSE;
			}
		}
		break;
}
#ifdef DEBUG 
	printf("INFO: %d\n",info);
#endif DEBUG
lower_dtr(fd);
tcsetattr(fd, TCSAFLUSH, &optionsold);
close(fd);
unlock_port();
close(fileout);
return info;
} /* END main() */

/* ------------------------------------------------------------------------------ */
