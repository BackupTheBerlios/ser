/***************************************************************************

    @(#)$Id: wmr918util.c,v 1.1 2002/09/23 19:12:51 bogdan Rel $

                          wmr918util.c  -  description
                             -------------------
    begin                : Mon May 28 2001
    copyright            : (C) 2001 by Dominique Le Foll
    email                : dominique@le-foll.demon.co.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/



#include <stdio.h>
#include <unistd.h>
#include <string.h>
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
#include "wmr918.h"
#include "parse.h"

unsigned char wmr918groupbuf[WMR918GROUPS][WMR918GROUP_LENMAX];
int wmr918grouplen[] = WMR918GROUP_LENGTHS;

void
checksum (unsigned char *buffer,int grouptype)
{ // calculate the checksum for the wx200 buffer
  int chksum, i;
  chksum = 0;
  for (i=0;i<=(grouplen[grouptype])-2;i++) // for all byte except the checksum itself
    chksum += (int) buffer[i];
  buffer[grouplen[grouptype]-1] = (unsigned char) (chksum & 0x00ff);
}

int
wmr918parse (int grouptype)   // convert the wmr918 buffer in wx200 buffer
{
  unsigned char *buf200a, *buf200b, *buf200c;  // short to wx200 buffer
  unsigned char *buf918; // short to wmr918 buffer

  int i, baro, baros, wx200type, group, ret;
  unsigned char pressure[4]; //coded abcd  -> abcd
  unsigned char seapressure[7];//coded abc.d or abcd.ef -> abcd.e
  unsigned char clock[2]; // used for hh, mm, ss conversion
  float barosea;
  time_t nowtime;
  struct tm *now = NULL;

/*  	    if (verbose) */
/*  	    	printf ("wx200d debug WMR918 GROUPNUM = %d \n",grouptype); */
  switch (grouptype) {
  case WMR918GROUP0:         // anemometer and wind related data
    {  
      buf918 = wmr918groupbuf[WMR918GROUP0];
      buf200a = groupbuf[GROUPC]; // equivalent to GROUPC

      buf200a[1] = MHI(LO(buf918[4])) | HI(buf918[3]);
      buf200a[2] = MHI(LO(buf918[2])) | HI(buf918[4]);
      buf200a[3] = MHI(LO(buf918[3])) | HI(buf918[2]);
      buf200a[4] = buf918[5];
      // average direction not provided uses gust in place
      buf200a[5] = MHI(LO(buf918[2])) | (LO(buf918[6]));
      buf200a[6] = MHI(LO(buf918[3])) | (HI(buf918[2]));
      for (i=7;i<=14;i++) // equivalent should be calculated by SW
        buf200a[i] = 0;   // not supported by WMR 918
      buf200a[15] = 0x02; // speeds are always in m/s
      buf200a[16] = buf918[7];
      // lo of chill to be done later by SW
      for (i=17;i<=20;i++)
        buf200a[i] = 0;
      buf200a[21] = MHI(BIT((buf918[6]),3)) << 1;
      // functions not provided by WMR918
      for (i=22;i<=25;i++)
        buf200a[i] = 0;
      buf200a[23] = MHI(LO(wmr918groupbuf[WMR918GROUPF][1]));// batterie status
      checksum (buf200a,GROUPC);
    	wx200type = GROUPC;
    	break;
    }
  case WMR918GROUP1:         // rain guage
    { buf918 = wmr918groupbuf[WMR918GROUP1];
      buf200a = groupbuf[GROUPB]; // equivalent to GROUPB

      buf200a[1] = buf918[2];
      buf200a[2] = buf918[3];
      buf200a[3] = buf918[6];
      buf200a[4] = buf918[7];
      buf200a[5] = buf918[4];
      buf200a[6] = buf918[5];
      buf200a[7] = buf918[8];
      buf200a[8] = buf918[9];
      buf200a[9] = buf918[10];
      buf200a[10] = NUM(buf918[11]) & 0xff; //wmr918 is always in mm
      // nor supported by WMR918
      buf200a[11] = 0;
      buf200a[12] = 0;

      checksum (buf200a,GROUPB);
      wx200type = GROUPB;
      break;
}
  case WMR918GROUP2:         // extra sensors
    { wx200type = -1;       // no known equivalent
      break;
   }
  case WMR918GROUP3:         // outside temp, humidity and dewpoint
    {
      buf918 = wmr918groupbuf[WMR918GROUP3];
      buf200a = groupbuf[GROUP8];// part equivalent GROUP9, GROUP8 and GROUPF
      buf200b = groupbuf[GROUP9];
      buf200c = groupbuf[GROUPA];

      buf200b[16] = buf918[2];
      buf200b[17] = LO(buf918[3]) | MHI(BIT(HI(buf918[3]),3));
      // high and lo with date to be done by SW

      buf200a[20] = buf918[4];
      // high and lo with date to be done by SW

      buf200c[18] = buf918[5];
      // high and lo with date to be done by SW

      checksum(buf200a,GROUP9);
      checksum(buf200b,GROUP8);
      checksum(buf200c,GROUPA);
			wx200type = GROUPS;

      break;
   }
  case WMR918GROUP5:         // inside temp, humidity, dewpoint, and baro
    {
      buf918 = wmr918groupbuf[WMR918GROUP5];
      buf200a = groupbuf[GROUP9];    // part equivalent to GROUP9, GROUP8 and GROUPA
      buf200b = groupbuf[GROUP8];
      buf200c = groupbuf[GROUPA];

      buf200a[1] = buf918[2];
      buf200a[2] = (LO(buf918[3]) & 0x07) | (LO(buf918[3]) & 0x04);

      buf200b[8] = buf918[4];
      // high and low with date to be done by SW

      buf200c[7] = buf918[5];

      // high and low with date to be done by SW

      baro = buf918[6]+795;   // pressure in mb
      sprintf(pressure,"%4u",baro);
      buf200c[1] = (MHI(pressure[2] & 0x0F)) | (pressure[3] & 0x0F);
      buf200c[2] = (MHI(pressure[0] & 0x0F)) | (pressure[1] & 0x0F);

      switch (LO(buf918[7]))
      			{ //raising falling trend remain to be done by SW
      				case 0x0c:
      				 buf200c[6] = 1;	//clear
      					break;
      				case 0x06:
      				  buf200c[6] = 4; //partly cloudy
      					break;
      				case 0x02:
      				  buf200c[6] = 2; //cloudy
      					break;
      				case 0x03:
      				  buf200c[6] = 8; //rain
      					break;
      				}	
      // see level references format abc.d to be added to baro
			seapressure[0] = HI(buf918[9]) | 0x30; // a
			seapressure[1] = LO(buf918[9]) | 0x30; // b
			seapressure[2] = HI(buf918[8]) | 0x30; // c
			seapressure[3] = LO(buf918[8]) | 0x30; // d
			sscanf (seapressure,"%4u",&baros);
			barosea = baro + baros/10 - 795;		//sea ref is formated as abc.d
			sprintf(seapressure,"%4.1f",barosea);
			buf200c[3] = (MHI(seapressure[3] & 0x0F)) | (seapressure[5] & 0x0F);
			buf200c[4] = (MHI(seapressure[1] & 0x0F)) | (seapressure[2] & 0x0F);
			buf200c[5] = 0x20 | seapressure[0];	//wmr918 is always in mb
				
			
			
      checksum(buf200a,GROUP9);
      checksum(buf200b,GROUP8);
      checksum(buf200c,GROUPA);
			wx200type = GROUPS;
      break;
    }
  case WMR918GROUP6:         // inside temp, humidity, dewpoint, and baro for WMR968
    {
      buf918 = wmr918groupbuf[WMR918GROUP6];
      buf200a = groupbuf[GROUP9];    // part equivalent to GROUP9, GROUP8 and GROUPA
      buf200b = groupbuf[GROUP8];
      buf200c = groupbuf[GROUPA];

      buf200a[1] = buf918[2];
      buf200a[2] = (LO(buf918[3]) & 0x07) | (LO(buf918[3]) & 0x04);

      buf200b[8] = buf918[4];
      // high and low with date to be done by SW

      buf200c[7] = buf918[5]; //wmr918 has only one dewling point
			buf200c[18] = buf918[5];
      // high and low with date to be done by SW

      baro = (buf918[6] | (BIT(buf918[7],0) << 7)) + 600;   // pressure in mb
      sprintf(pressure,"%4u",baro);
      buf200c[1] = (MHI(pressure[1] & 0x0F)) | (pressure[0] & 0x0F);
      buf200c[2] = (MHI(pressure[3] & 0x0F)) | (pressure[2] & 0x0F);

      switch (LO(buf918[7]))
    			{ //raising falling trend remain to be done by SW
    				case 0x0c:
    				 buf200c[6] = 1;	//clear
    					break;
    				case 0x06:
    				  buf200c[6] = 4; //partly cloudy
    					break;
    				case 0x02:
    				  buf200c[6] = 2; //cloudy
    					break;
    				case 0x03:
    				  buf200c[6] = 8; //rain
    					break;
					}
      // see level references format abcd.ef to be added to baro
      // only abcd.e can be used for compatibility with wx200

			seapressure[0] = HI(buf918[10]) | 0x30; // a
			seapressure[1] = LO(buf918[10]) | 0x30; // b
			seapressure[2] = HI(buf918[9]) | 0x30; // c
			seapressure[3] = LO(buf918[9]) | 0x30; // d
 			seapressure[4] = HI(buf918[8]) | 0x30; // e
			seapressure[5] = LO(buf918[8]) | 0x30; // f
			sscanf (seapressure,"%6u",&baros);
			barosea = baro + baros/100 - 600;		//sea ref is formated as abc.d
			sprintf(seapressure,"%4.1f",barosea); //leave 1/100 for compatibility
			buf200c[3] = (MHI(seapressure[3] & 0x0F)) | (seapressure[5] & 0x0F);
			buf200c[4] = (MHI(seapressure[1] & 0x0F)) | (seapressure[2] & 0x0F);
			buf200c[5] = 0x20 | seapressure[0];	//wmr918 is always in mb
		
      checksum(buf200a,GROUP9);
      checksum(buf200b,GROUP8);
      checksum(buf200c,GROUPA);
			wx200type = GROUPS;
			break;
    }
  case WMR918GROUPE:			// sequence number ( minutes )
  case WMR918GROUPF:      // hourly report ( hour )
    {                     // part equivalent to GROUPF and GROUP8
      // wmr918 clock is not used but system clock instead
     	buf200a = groupbuf[GROUP8];
     	buf200b = groupbuf[GROUPF];
     	   	
     	nowtime = time(NULL);
     	now = localtime(&nowtime);
     	wx.td.clock.sec = now->tm_sec;
     	sprintf (clock,"%2u",now->tm_sec);
     	buf200a[1] = (MHI(clock[0] & 0x0F)) | (clock[1] & 0x0F);
     	wx.td.clock.min = now->tm_min;
     	sprintf (clock,"%2u",now->tm_min);
     	buf200a[2]= (MHI(clock[0] & 0x0F)) | (clock[1] & 0x0F);
     	wx.td.clock.hour = now->tm_hour;
     	sprintf (clock,"%2u",now->tm_hour);
     	buf200a[3] = (MHI(clock[0] & 0x0F)) | (clock[1] & 0x0F);
     	wx.td.clock.day = now->tm_mday;
     	sprintf (clock,"%2u",now->tm_mday);
     	buf200a[4] = (MHI(clock[0] & 0x0F)) | (clock[1] & 0x0F);
     	wx.td.clock.mon = now->tm_mon + 1; //month range 0-11 under unix
     	buf200a[5] = (MHI(0x01)) | (wx.td.clock.mon); // 24 hours / DD MM format hardwired
     	
     	// copy date and time to F type for wx200 client compatibility
     	memcpy(&buf200b[1], &buf200a[1], 3);
     	
     	checksum(buf200b,GROUPF);
     	checksum(buf200a,GROUP8);
     	wx200type = GROUP8;
			break;
     }

  default:
    {     // ignore any other groupe type
		wx200type = -1;
    break;
		}
  }
  ret = -1;			/* return positive only if any bit changed */
  group = wx200type;
  if (group == GROUP8) {	/* only a time change? */
    if (!memcmp(&groupbuf[GROUP8][4], &groupbuf[GROUP8 + GROUPS][4], 30))
      group = GROUPF;
  }
  if (group == GROUPS) {	/* HACK: multiple group change */
    ret = 0;
    for (i = 0; i < 3; i++) {
      group = GROUP8 + i;
      if (memcmp(groupbuf[group], groupbuf[group + GROUPS], grouplen[group]))
	ret |= (0x01 << i);	/* set bits of changed groups */
      memcpy(groupbuf[group + GROUPS], groupbuf[group], grouplen[group]);
    }
  } else {
    if (memcmp(groupbuf[group], groupbuf[group + GROUPS], grouplen[group]))
      ret = group;
    memcpy(groupbuf[group + GROUPS], groupbuf[group], grouplen[group]);
  }
  return ret;
}

int
wmr918bufread (int fd)
{
  unsigned char syncbuf[3];
  int retval, i, grouptype, cs;

  /* read the first three bytes -- ff ff type */
  retval = read(fd, &syncbuf[0], 3 );
  if (retval !=3)
	      return (-2);              // error reading serial port
  while ((syncbuf[0] != 0xff) || (syncbuf[1] != 0xff)
         || (syncbuf[2] > 15)) {
    syncbuf[0] = syncbuf[1];
    syncbuf[1] = syncbuf[2];
    retval = read(fd, &syncbuf[2], 1 );
   if (retval !=1)
     return (-2);              // error reading serial port
  }
  grouptype =  (int)syncbuf[2];
  wmr918groupbuf[grouptype][0] = grouptype; // place group type

  // read remaining bytes of this type + checksum
  retval = read(fd, &wmr918groupbuf[grouptype][1],(wmr918grouplen[grouptype]));

  // Control checksum
  cs = 2*0xff;                // sync FF are part of the Chksum
  for (i=0; i<wmr918grouplen[grouptype]; i++)
    cs += wmr918groupbuf[grouptype][i];
  cs = cs & 0xFF;
  if (cs != wmr918groupbuf[grouptype][(wmr918grouplen[grouptype])])
	  return (-2);       // checksum is wrong

  // checksum is OK pqrse data and return wx200 group type
  return (wmr918parse(grouptype));
}
