/***************************************************************************

    @(#)$Id: wmr918.h,v 1.1 2002/09/23 19:12:51 bogdan Rel $

                          wmr918.h  -  description
                             -------------------
    begin                : Sun May 13 2001
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

/*   wmr918 related declaration */

#define WMR918GROUPS 16

#define WMR918GROUP0	0	/* 00 - anemometer and wind related data */
#define WMR918GROUP1	1       /* 01 - rain guage */
#define WMR918GROUP2	2       /* 02 - extra sensors */
#define WMR918GROUP3	3       /* 03 - outside temp, humidity and dewpoin */
#define WMR918GROUP4	4       /* 04 - unknow */
#define WMR918GROUP5	5       /* 05 - inside temp, humidity, dewpoint, and baro */
#define WMR918GROUP6	6       /* 06 - inside temp, humidity, dewpoint, baro for
				   wmr968 and some wmr918's */
#define WMR918GROUP7	7       /* 07 - unknown */
#define WMR918GROUP8	8       /* 08 - unknown */	
#define WMR918GROUP9	9       /* 09 - unknown */	
#define WMR918GROUPA	10      /* 0a - unknown */	
#define WMR918GROUPB	11      /* 0b - unknown */	
#define WMR918GROUPC	12      /* 0c - unknown */	
#define WMR918GROUPD	13      /* 0d - unknown */	
#define WMR918GROUPE	14	/* 0e - sequence number */
#define WMR918GROUPF	15      /* 0f - hourly status report */	

#define WMR918GROUP_LENGTHS {8,13,6,6,0,10,11,0,0,0,0,0,0,0,2,6}
#define WMR918GROUP_LENMAX 13


extern int wmr918bufread(int);  // wmr918util.c
extern int wmr918parse(int);    
extern unsigned char wmr918groupbuf[WMR918GROUPS][WMR918GROUP_LENMAX];
extern int wmr918grouplen[WMR918GROUPS];
