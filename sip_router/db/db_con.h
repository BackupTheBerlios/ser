/* 
 * $Id: db_con.h,v 1.2 2002/08/12 11:23:17 janakj Exp $ 
 */


#ifndef DB_CON_H
#define DB_CON_H


/*
 * This structure represents a database connection
 * and pointer to this structure is used as a connection
 * handle
 */
typedef struct {
	char* table;           /* Default table to use */
	int connected;         /* 1 if database is connected */
	unsigned char tail[0]; /* Variable length tail
				* database module specific */    
} db_con_t;


#define CON_CONNECTED(cn)  ((cn)->connected)
#define CON_TABLE(cn)      ((cn)->table)
#define CON_TAIL(cn)       ((cn)->tail)


int use_table(db_con_t* _h, const char* _t);


#endif /* DB_CON_H */
