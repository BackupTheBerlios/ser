/*
 * @(#)$Id: pg_api.c,v 1.1 2002/09/23 19:12:51 bogdan Rel $
 *
 * Copyright (C) 2001 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements Postgres database logging for wx200d
 *
 * by Raul Luna <rlunaro@bigfoot.com>, 2001/05/12
 *
 */

#include <config.h>
#include <string.h>
#include <sys/timeb.h>
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
#include <syslog.h>
#include "wx200.h"
#include "pg_api.h"


/* deletes a char from szIn and returns a pointer to szIn */
static char *pg_strchrdelete( const char *szIn, char *szOut, char cDel )
{
  const char *pszIn;
  char *pszOut;

  pszIn = szIn;
  pszOut = szOut;
  while( *pszIn )
  {  /* copy in to out only if not equal to the cDel char */
     if( *pszIn != cDel )
       *pszOut++ = *pszIn++;
     else
       pszIn++;
  }
  /* write the final "0" */
  *pszOut = *pszIn;

  return szOut;
}

/* connect to the database: return TRUE if connected,
   false if not connected */
PGconn *pg_init( const char *szConnInfo, const char *szTableName )
{
  char szTable[PG_MAX_TABLE_NAME];
  char szCommand[PG_MAX_COMMAND];
  char szErrorMsg[PG_MAX_COMMAND];
  PGconn *pConn;

  PGresult *pResult, *pCreate;

  pConn = PQconnectdb( szConnInfo );

  if( PQstatus( pConn ) == CONNECTION_OK )
  { /* ----->>> test if exist the table <<<-------------- */
    /* remove the '\'' characters -could be problematic in table names- */
    pg_strchrdelete( szTableName, szTable, '\'' );
    sprintf( szCommand, "select relname from pg_class where relname = '%s'", szTable );
    pResult = PQexec( pConn, szCommand );
    if( PQresultStatus( pResult ) == PGRES_TUPLES_OK )
    { if( PQntuples( pResult ) == 0 )
      { /* create table szTableName */
#ifdef PG_SIMPLE      	
      	sprintf( szCommand, "create table %s ( t           datetime,"
      					     " humid_in_val   float,"
      					     " humid_out_val  float,"
      					     " temp_in_val    float,"
      					     " temp_out_val   float,"
      					     " baro_local_val float,"
      					     " baro_sea_val   float,"
      					     " dew_in_val     float,"
      					     " dew_out_val    float,"
      					     " rain_val       float,"
      					     " wind_speed_val float,"
      					     " wind_dir_val   float,"
      					     " wind_chill_val float)", szTable );
#else
      	sprintf( szCommand, "create table %s ( t           datetime,"
      					     " humid_in_val   float,"
      					     " humid_out_val  float,"
      					     " temp_in_val    float,"
      					     " temp_out_val   float,"
      					     " baro_local_val float,"
      					     " baro_sea_val   float,"
      					     " dew_in_val     float,"
      					     " dew_out_val    float,"
      					     " rain_val       float,"
      					     " wind_speed_val float,"
      					     " wind_dir_val   float,"
      					     " wind_chill_val float,"
      					     " humid_in_or    int2,"
      					     " humid_out_or   int2,"
      					     " temp_in_or     int2,"
      					     " temp_out_or    int2,"
      					     " baro_local_or  int2,"
      					     " baro_sea_or    int2,"
      					     " dew_in_or      int2,"
      					     " dew_out_or     int2,"
      					     " rain_or        int2,"
      					     " wind_speed_or  int2,"
      					     " wind_dir_or    int2,"
      					     " wind_chill_or  int2,"
      					     " humid_in_err   int2,"
      					     " humid_out_err  int2,"
      					     " temp_in_err    int2,"
      					     " temp_out_err   int2,"
      					     " baro_local_err int2,"
      					     " baro_sea_err   int2,"
      					     " dew_in_err     int2,"
      					     " dew_out_err    int2,"
      					     " rain_err       int2,"
      					     " wind_speed_err int2,"
      					     " wind_dir_err   int2,"
      					     " wind_chill_err int2)", szTable );
#endif
  	pCreate = PQexec( pConn, szCommand );
  	if( PQresultStatus( pCreate ) != PGRES_COMMAND_OK )
  	{ sprintf( szErrorMsg, "(%s):%s", PG_API_NAME, PQerrorMessage( pConn ) );
  	  syslog( LOG_WARNING, szErrorMsg );
  	}
  	PQclear( pCreate );
      }
    }
    else
    { sprintf( szErrorMsg, "(%s):%s", PG_API_NAME, PQerrorMessage( pConn ) );
      syslog( LOG_WARNING, szErrorMsg );
    }
    PQclear( pResult );
  }

  return pConn;

}

/* inserts a record into the database */
int pg_insert( PGconn *pConn, const char *szTableName, WX *wx, int use_localtime )
{
  PGresult *pInsert;
  char szTable[PG_MAX_TABLE_NAME];
  char szCommand[PG_MAX_COMMAND];
  char szTimeBuf[30];
  char szErrorMsg[PG_MAX_COMMAND];
  time_t tNow;
  struct tm *pstNow;
  int nRet; 

  if( PQstatus( pConn ) == CONNECTION_OK )
  { nRet = 0;
    /* remove the '\'' characters -could be problematic in table names- */
    pg_strchrdelete( szTableName, szTable, '\'' );
    /* get the current date & time (localtime) */
    time( &tNow );
    if( use_localtime )
      pstNow = localtime( &tNow );
    else
      pstNow = gmtime( &tNow );
    sprintf( szTimeBuf, "%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d",
  		1900+pstNow->tm_year,
  		pstNow->tm_mon+1,
  		pstNow->tm_mday,
  		pstNow->tm_hour,
  		pstNow->tm_min,
  		pstNow->tm_sec );

#ifdef PG_SIMPLE
    sprintf( szCommand, "insert into %s"
				"( t,"
				" humid_in_val,"
	      			" humid_out_val,"
      		     		" temp_in_val,"
      		     		" temp_out_val,"
      		     		" baro_local_val,"
      		     		" baro_sea_val,"
      		     		" dew_in_val,"
      				" dew_out_val,"
      				" rain_val,"
      				" wind_speed_val,"
      				" wind_dir_val,"
      				" wind_chill_val )"
      			" values ( '%s',"
      				" %f,"		/* humid_in_val  */
      				" %f,"		/* humid_out_val */
      				" %f,"		/* temp_in_val 	 */
      				" %f,"		/* temp_out_val  */
      				" %f,"		/* baro_local_val*/
      				" %f,"		/* baro_sea_val  */
      				" %f,"		/* dew_in_val 	 */
      				" %f,"		/* dew_out_val   */
      				" %f,"		/* rain_val      */
      				" %f,"		/* wind_speed_val*/
      				" %f,"		/* wind_dir_val  */
      				" %f)",		/* wind_chill_val*/
    				szTable,
    				szTimeBuf,
    				wx->humid.in.n,
    				wx->humid.out.n,
    				wx->temp.in.n,
    				wx->temp.out.n,
    				wx->baro.local.n,
    				wx->baro.sea.n,
    				wx->dew.in.n,
    				wx->dew.out.n,
    				wx->rain.rate.n,
    				wx->wind.gspeed.n,
    				wx->wind.gdir.n,
    				wx->chill.temp.n );
#else
    sprintf( szCommand, "insert into %s"
				"( t,"
				" humid_in_val,"
	      			" humid_out_val,"
      		     		" temp_in_val,"
      		     		" temp_out_val,"
      		     		" baro_local_val,"
      		     		" baro_sea_val,"
      		     		" dew_in_val,"
      				" dew_out_val,"
      				" rain_val,"
      				" wind_speed_val,"
      				" wind_dir_val,"
      				" wind_chill_val,"
				" humid_in_or,"
	      			" humid_out_or,"
      		     		" temp_in_or,"
      		     		" temp_out_or,"
      		     		" baro_local_or,"
      		     		" baro_sea_or,"
      		     		" dew_in_or,"
      				" dew_out_or,"
      				" rain_or,"
      				" wind_speed_or,"
      				" wind_dir_or,"
      				" wind_chill_or,"
				" humid_in_err,"
	      			" humid_out_err,"
      		     		" temp_in_err,"
      		     		" temp_out_err,"
      		     		" baro_local_err,"
      		     		" baro_sea_err,"
      		     		" dew_in_err,"
      				" dew_out_err,"
      				" rain_err,"
      				" wind_speed_err,"
      				" wind_dir_err,"
      				" wind_chill_err )"
      			" values ( '%s',"
      				" %f,"		/* humid_in_val  */
      				" %f,"		/* humid_out_val */
      				" %f,"		/* temp_in_val 	 */
      				" %f,"		/* temp_out_val  */
      				" %f,"		/* baro_local_val*/
      				" %f,"		/* baro_sea_val  */
      				" %f,"		/* dew_in_val 	 */
      				" %f,"		/* dew_out_val   */
      				" %f,"		/* rain_val      */
      				" %f,"		/* wind_speed_val*/
      				" %f,"		/* wind_dir_val  */
      				" %f,"		/* wind_chill_val*/
      				" %d,"		/* humid_in_or   */
      				" %d,"		/* humid_out_or  */
      				" %d,"		/* temp_in_or 	 */
      				" %d,"		/* temp_out_or   */
      				" %d,"		/* baro_local_or */
      				" %d,"		/* baro_sea_or   */
      				" %d,"		/* dew_in_or 	 */
      				" %d,"		/* dew_out_or    */
      				" %d,"		/* rain_or       */
      				" %d,"		/* wind_speed_or */
      				" %d,"		/* wind_dir_or   */
      				" %d,"		/* wind_chill_or */
      				" %d,"		/* humid_in_err  */
      				" %d,"		/* humid_out_err */
      				" %d,"		/* temp_in_err 	 */
      				" %d,"		/* temp_out_err  */
      				" %d,"		/* baro_local_err*/
      				" %d,"		/* baro_sea_err  */
      				" %d,"		/* dew_in_err 	 */
      				" %d,"		/* dew_out_err   */
      				" %d,"		/* rain_err      */
      				" %d,"		/* wind_speed_err*/
      				" %d,"		/* wind_dir_err  */
      				" %d)",		/* wind_chill_err*/
    				szTable,
    				szTimeBuf,
    				wx->humid.in.n,
    				wx->humid.out.n,
    				wx->temp.in.n,
    				wx->temp.out.n,
    				wx->baro.local.n,
    				wx->baro.sea.n,
    				wx->dew.in.n,
    				wx->dew.out.n,
    				wx->rain.rate.n,
    				wx->wind.gspeed.n,
    				wx->wind.gdir.n,
    				wx->chill.temp.n,
    				wx->humid.in.or,
    				wx->humid.out.or,
    				wx->temp.in.or,
    				wx->temp.out.or,
    				wx->baro.local.or,
    				wx->baro.sea.or,
    				wx->dew.in.or,
    				wx->dew.out.or,
    				wx->rain.rate.or,
    				wx->wind.gspeed.or,
    				wx->wind.gdir.or,
    				wx->chill.temp.or,
    				wx->humid.in.err,
    				wx->humid.out.err,
    				wx->temp.in.err,
    				wx->temp.out.err,
    				wx->baro.local.err,
    				wx->baro.sea.err,
    				wx->dew.in.err,
    				wx->dew.out.err,
    				wx->rain.rate.err,
    				wx->wind.gspeed.err,
    				wx->wind.gdir.err,
    				wx->chill.temp.err );
#endif
    pInsert = PQexec( pConn, szCommand );
    if( PQresultStatus( pInsert ) != PGRES_COMMAND_OK )
    { sprintf( szErrorMsg, "(%s):%s", PG_API_NAME, PQerrorMessage( pConn ) );
      syslog( LOG_WARNING, szErrorMsg );
      nRet = -1; 
    }
  }
  else 
  { /* the connection was no good: retry */
    sprintf( szErrorMsg, "(%s):%s", PG_API_NAME, "The connection was no good: retying..." );
    syslog( LOG_WARNING, szErrorMsg );
    nRet = -2;
  }

  return nRet;
}

/* disconnect from the database */
void pg_finish( PGconn *pConn )
{
  PQfinish( pConn );
}


