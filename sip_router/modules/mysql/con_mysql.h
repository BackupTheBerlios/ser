/* 
 * $Id: con_mysql.h,v 1.2 2002/09/19 11:51:26 jku Exp $
 *
 */

#ifndef CON_MYSQL_H
#define CON_MYSQL_H

#include <mysql/mysql.h>

/*
 * MySQL specific connection data
 */
struct con_mysql {
	MYSQL_RES* res; /* Actual result */
	MYSQL* con;     /* Connection representation */
	MYSQL_ROW row;  /* Actual row in the result */
};


#define CON_RESULT(db_con)     (((struct con_mysql*)((db_con)->tail))->res)
#define CON_CONNECTION(db_con) (((struct con_mysql*)((db_con)->tail))->con)
#define CON_ROW(db_con)        (((struct con_mysql*)((db_con)->tail))->row)


#endif /* CON_MYSQL_H */
