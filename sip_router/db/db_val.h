/* 
 * $Id: db_val.h,v 1.4 2002/08/12 11:23:17 janakj Exp $ 
 */

#ifndef DB_VAL_H
#define DB_VAL_H

#include <time.h>
#include "../str.h"


/*
 * Accepted column types
 */
typedef enum {
	DB_INT,
        DB_DOUBLE,
	DB_STRING,
	DB_STR,
	DB_DATETIME,
	DB_BLOB
} db_type_t;


/*
 * Column value structure
 */
typedef struct {
	db_type_t type;                  /* Type of the value */
	int nul;                         /* Means that the column in database has no value */
	union {
		int          int_val;    /* integer value */
		double       double_val; /* double value */
		time_t       time_val;   /* unix time value */
		const char*  string_val; /* NULL terminated string */
		str          str_val;    /* str string value */
		str          blob_val;   /* Blob data */
	} val;                           /* union of all possible types */
} db_val_t;


/*
 * Useful macros for accessing attributes of db_val structure
 */

#define VAL_TYPE(dv)   ((dv)->type)
#define VAL_NULL(dv)   ((dv)->nul)
#define VAL_INT(dv)    ((dv)->val.int_val)
#define VAL_DOUBLE(dv) ((dv)->val.double_val)
#define VAL_TIME(dv)   ((dv)->val.time_val)
#define VAL_STRING(dv) ((dv)->val.string_val)
#define VAL_STR(dv)    ((dv)->val.str_val)
#define VAL_BLOB(dv)   ((dv)->val.blob_val)


/*
 * Convert string to given type
 */
int str2val(db_type_t _t, db_val_t* _v, const char* _s, int _l);


/*
 * Convert given type to string
 */
int val2str(db_val_t* _v, char* _s, int* _len);


#endif /* DB_VAL_H */
