/* 
 * $Id: db_mod.c,v 1.5 2002/03/01 10:51:03 janakj Exp $ 
 */

#include "../../sr_module.h"
#include "dbase.h"
#include <stdio.h>

/*
 * MySQL database module interface
 */

static struct module_exports mysql_exports = {	
	"mysql",
	(char*[]) {
		"db_use_table",
		"db_init",
		"db_close",
		"db_query",
		"db_free_query",
		"db_insert",
		"db_delete",
		"db_update"
	},
	(cmd_function[]) {
		(cmd_function)use_table,
		(cmd_function)db_init,
		(cmd_function)db_close,
		(cmd_function)db_query,
		(cmd_function)db_free_query,
		(cmd_function)db_insert,
		(cmd_function)db_delete,
		(cmd_function)db_update
	},
	(int[]) {
                2, 1, 2, 2, 2, 2, 2, 2
	},
	(fixup_function[]) {
		0, 0, 0, 0, 0, 0, 0, 0
	},
	8, /* number of functions*/
	0, /* response function*/
	0,  /* destroy function */
	0,	/* oncancel function */
	0   /* per-child init function */
};


#ifdef STATIC_MYSQL
struct module_exports* mysql_mod_register()
#else
struct module_exports* mod_register()
#endif
{
	fprintf(stderr, "mysql - registering...\n");
	return &mysql_exports;
}
