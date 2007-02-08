/*
 * $Id: ser_objdump.c,v 1.2 2007/02/08 17:33:18 janakj Exp $
 *
 * Copyright (C) 2007 iptel.org
 *
 * ser_objdump is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * ser_objdump is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define _GNU_SOURCE
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <bfd.h>
#include "select.h"
#include "ser_objdump.h"

#define PARAM_STRING     (1U<<0)	/* String (char *) parameter type */
#define PARAM_INT        (1U<<1)	/* Integer parameter type */
#define PARAM_STR        (1U<<2)	/* struct str parameter type */
#define PARAM_USE_FUNC   (1U<<(8*sizeof(int)-1))

#define FUNC_REQUEST_ROUTE (1U<<0)	/* Function can be used in request 
					 * route blocks */
#define FUNC_FAILURE_ROUTE (1U<<1)	/* Function can be used in reply
					 * route blocks */
#define FUNC_ONREPLY_ROUTE (1U<<2)	/* Function can be used in
					 * on_reply */
#define FUNC_BRANCH_ROUTE  (1U<<3)	/* Function can be used in
					 * branch_route blocks */
#define FUNC_ONSEND_ROUTE  (1U<<4)	/* Function can be used in
					 * onsend_route blocks */

/*
 * This is a temporary table that maps modules to the names
 * of symbols containing select tables. This is needed because
 * we currently do not have a pointer to the select table in
 * exports structure (to be fixed in the future)
 */
struct select_table_map {
	char* module;
	char* symbol_name;
} sel_map[] = {{"core",      "select_core"},
			   {"db_ops",    "sel_declaration"},
			   {"eval",      "sel_declaration"},
			   {"nathelper", "sel_declaration"},
			   {"rr",        "rr_select_table"},
			   {"textops",   "sel_declaration"},
			   {"timer",     "sel_declaration"},
			   {"tls",       "tls_sel"},
			   {"xmlrpc",    "xmlrpc_sel"},
			   {NULL,        NULL}
};


char* lookup_sel_table(char* module)
{
	int i;

	for(i = 0; sel_map[i].module; i++) {
		if (!strcmp(module, sel_map[i].module)) {
			return sel_map[i].symbol_name;
		}
	}
	return NULL;
}

struct res res;

struct section {
    asection       *hdr;
    bfd_byte       *contents;
};


struct {
    char           *filename;
    bfd            *handle;
    asymbol       **symtab;
    int             syms_no;
} module;


/*
 * Module API structures cut from SER sources 
 */
struct cmd_export {
    char           *name;	/* null terminated command name */
    void           *function;	/* pointer to the corresponding function */
    int             param_no;	/* number of parameters used by the
				 * function */
    void           *fixup;	/* pointer to the function called to "fix" 
				 * the parameters */
    int             flags;	/* Function flags */
};


/*
 * Remote Procedure Call Export
 */
struct rpc_export {
    const char     *name;	/* Name of the RPC function (null
				 * terminated) */
    void           *function;	/* Pointer to the function */
    const char    **doc_str;	/* Documentation strings, method signature 
				 * and description */
    unsigned int    flags;	/* Various flags, reserved for future use */
} rpc_export_t;


struct param_export {
    char           *name;	/* null terminated param. name */
    unsigned int    type;	/* param. type */
    void           *param_pointer;	/* pointer to the param. memory
					 * location */
};


struct module_exports {
    char           *name;	/* null terminated module name */
    struct cmd_export *cmds;	/* null terminated array of the exported
				 * commands */
    struct rpc_export *rpc_methods;	/* null terminated array of
					 * exported rpc methods */
    struct param_export *params;	/* null terminated array of the
					 * exported module parameters */
};


int             print_all = 1;	/* Print all symbols by default */
int             print_funcs = 0;
int             print_params = 0;
int             print_rpc = 0;
int             print_selects = 0;
int             out_fmt = FMT_CSV;	/* The default output format is
					 * CSV */
char           *version_string = NULL;
char           *table = "ser_symbol";
char           *prog_name = NULL;
char           *mod_name = NULL;


static struct option long_options[] = {
    {"functions", no_argument, NULL, 'f'},
    {"parameters", no_argument, NULL, 'p'},
    {"selects", no_argument, NULL, 'S'},
    {"rpc", no_argument, NULL, 'r'},
    {"csv", no_argument, NULL, 'c'},
    {"sql", no_argument, NULL, 's'},
    {"table", required_argument, NULL, 't'},
    {"module", required_argument, NULL, 'm'},
    {"help", no_argument, NULL, 'h'},
    {"version", required_argument, NULL, 'v'},
    {0, no_argument, 0, 0}
};


static void
usage(FILE * stream, int status)
{
    fprintf(stream, "Usage: %s [option(s)] <module>\n", prog_name);
    fprintf(stream, " List SER symbols in <module>.\n");
    fprintf(stream, " The options are:\n\
  -f, --functions      List module function symbols (on by default)\n\
  -p, --parameters     List module parameter symbols (on by default)\n\
  -S, --selects        List selects exported by module (on by default)\n\
  -r, --rpc            List module RPC functions (on by default)\n\
  -c, --csv            Display output in CSV format (default)\n\
  -s, --sql            Display output in SQL format\n\
  -t, --table <str>    Table name to be used in SQL format\n\
  -m, --module <str>   Set module name to <str>\n\
  -h, --help           Display this information\n\
  -v, --version <str>  Set version string to <str>\n\
\n");
    if (status == 0) {
	fprintf(stream, "Report bugs to serdev@lists.iptel.org.\n");
	fprintf(stream, "\nKnown issues:\n");
	fprintf(stream, "  Pointers to in select tables must point to static functions\n");
	fprintf(stream, "  otherwise the linker would set the pointers to NULL and\n");
	fprintf(stream, "  ser_objdump will report an error\n\n");
    }
    exit(status);
}


static          bfd_boolean
section_match(bfd * abfd, asection * sect, void *addr)
{
    return (sect->vma <= (unsigned long) addr
	    && (unsigned long) addr < sect->vma + sect->size);
}


unsigned long
relocate(void *addr)
{
    asection       *section;

    if (addr == 0)
	return 0;

    section = bfd_sections_find_if(module.handle, section_match, addr);
    if (section == NULL) {
	fprintf(stderr, "Could not find section for address %p\n", addr);
	exit(1);
    }

    return (unsigned long) addr - (unsigned long) (section->vma) +
	(unsigned long) section->contents;
}


static void
parse_cmd(struct res *res, struct cmd_export *cmd)
{
    res->name = (char *) relocate(cmd->name);
    res->type = TYPE_FUNC;
    res->params = cmd->param_no;
    res->flags = cmd->flags;
    res->doc = NULL;
}


static void
parse_rpc(struct res *res, struct rpc_export *rpc)
{
    res->name = (char *) relocate((void *) rpc->name);
    res->type = TYPE_RPC;
    res->params = 0;
    res->flags = rpc->flags;
    res->doc =
	(rpc->
	 doc_str ? (char *) relocate(((char **) relocate(rpc->doc_str))[0])
	 : NULL);
}


static void
parse_param(struct res *res, struct param_export *param)
{
    res->name = (char *) relocate(param->name);
    res->type = TYPE_PARAM;
    res->params = 0;
    res->flags = param->type;
    res->doc = NULL;
}

static void
print_res_csv(struct res *res)
{
    printf("%s,%s,%s,%d,%d,0x%x,%s\n",
	   mod_name ? mod_name : res->module,
	   version_string ? version_string : res->version,
	   res->name, res->type, res->params, res->flags, res->doc);
}

static void
print_res_sql(struct res *res)
{
    printf
	("INSERT INTO %s (module, version, name, type, params, flags, doc) VALUES ('%s','%s','%s',%d,%d,0x%x,",
	 table, mod_name ? mod_name : res->module,
	 version_string ? version_string : res->version, res->name,
	 res->type, res->params, res->flags);
    if (res->doc == NULL) {
	printf("NULL);\n");
    } else {
	printf("'%s');\n", res->doc);
    }
}


void print_res(struct res *res)
{
    if (out_fmt == FMT_CSV)
	print_res_csv(res);
    else
	print_res_sql(res);
}


static void
load_data_section(bfd * abfd, asection * sect, void *obj)
{
    bfd_byte       *buf;
    if (!bfd_malloc_and_get_section(abfd, sect, &buf)) {
	fprintf(stderr, "Error while loading section %s from %s\n",
		sect->name, module.filename);
	exit(1);
    }
    sect->contents = buf;
}


int
open_module(void)
{
    char          **matching;
    long            storage_needed;

    module.handle = bfd_openr(module.filename, NULL);
    if (module.handle == NULL) {
	fprintf(stderr, "Error while opening file %s\n", module.filename);
	return -1;
    }

    if (!bfd_check_format_matches(module.handle, bfd_object, &matching)) {
	fprintf(stderr, "File %s is not an object file\n",
		module.filename);
	return -1;
    }

    storage_needed = bfd_get_symtab_upper_bound(module.handle);
    if (storage_needed < 0) {
	fprintf(stderr, "Error while retrieving symbol table from %s\n",
		module.filename);
	return -1;
    }

    if (storage_needed == 0) {
	fprintf(stderr, "No symbol table found in %s\n", module.filename);
	return -1;
    }

    module.symtab = (asymbol **) malloc(storage_needed);
    module.syms_no = bfd_canonicalize_symtab(module.handle, module.symtab);
    if (module.syms_no < 0) {
	fprintf(stderr, "Error while retrieving symbol table from %s\n",
		module.filename);
	return -1;
    }

    if (module.syms_no == 0) {
	fprintf(stderr, "Symbol table appears to be empty in %s\n",
		module.filename);
	return -1;
    }

    bfd_map_over_sections(module.handle, load_data_section, NULL);
    return 0;
}


asymbol        *
get_module_symbol(char *name)
{
    int             i;

    for (i = 0; i < module.syms_no; i++) {
	if (!strcmp(module.symtab[i]->name, name))
	    return module.symtab[i];
    }
    return NULL;
}

char           *
get_module_version(void)
{
    asymbol        *version_sym;
    char           *module_version;

    version_sym = get_module_symbol("module_version");
    if (version_sym == NULL)
	return NULL;

    module_version =
	*(char **) (version_sym->section->contents + version_sym->value);
    return (char *) relocate(module_version);
}


struct module_exports *
get_module_exports(void)
{
    asymbol        *exports_sym;

    exports_sym = get_module_symbol("exports");
    if (exports_sym == NULL) {
		return NULL;
	}

    return (struct module_exports *) (exports_sym->section->contents +
				      exports_sym->value);
}


select_row_t* get_module_selects(char* name)
{
    asymbol        *selects_sym;

    selects_sym = get_module_symbol(name);
    if (selects_sym == NULL) {
		return NULL;
	}

    return (select_row_t*)(selects_sym->section->contents +
						   selects_sym->value);
}


int
main(int argc, char **argv)
{
    struct module_exports *exports;
    struct cmd_export *cmd;
    struct rpc_export *rpc;
	select_row_t* select;
    struct param_export *param;
    int             i,
                    c;
	char* s_table;

    bfd_init();

    prog_name = argv[0];

    while ((c =
	    getopt_long(argc, argv, "fprScst:m:hHv:", long_options,
			(int *) 0)) != EOF) {
	switch (c) {
	case 'f':
	    print_all = 0;
	    print_funcs = 1;
	    break;

	case 'p':
	    print_all = 0;
	    print_params = 1;
	    break;

	case 'r':
	    print_all = 0;
	    print_rpc = 1;
	    break;

	case 'S':
		print_all = 0;
		print_selects = 1;
		break;

	case 'c':
	    out_fmt = FMT_CSV;
	    break;

	case 's':
	    out_fmt = FMT_SQL;
	    break;

	case 't':
	    if (optarg == NULL) {
		usage(stderr, 1);
	    } else {
		table = optarg;
	    }
	    break;

	case 'm':
	    if (optarg == NULL) {
		usage(stderr, 1);
	    } else {
		mod_name = optarg;
	    }
	    break;

	case 'H':
	case 'h':
	    usage(stdout, 0);
	    break;

	case 'v':
	    if (optarg == NULL) {
		usage(stderr, 1);
	    } else {
		version_string = optarg;
	    }
	    break;

	default:
	    usage(stderr, 1);
	    break;
	}
    }

    if (optind >= argc) {
	usage(stderr, 1);
    }

    module.filename = argv[optind];

    if (open_module() < 0)
	exit(1);

    res.version = get_module_version();
    if (res.version == NULL) {
	fprintf(stderr, "Cannot retrieve module version from %s\n",
		module.filename);
	exit(1);
    }
    exports = get_module_exports();
    if (exports == NULL) {
	fprintf(stderr, "Module %s does not seem to be SER compatible\n",
		module.filename);
	exit(1);
    }

    res.module = (char *) relocate(exports->name);

    if (print_all || print_funcs) {
	cmd = (struct cmd_export *) relocate(exports->cmds);
	for (i = 0; cmd && cmd[i].name; i++) {
	    parse_cmd(&res, cmd + i);
	    print_res(&res);
	}
    }

    if (print_all || print_rpc) {
	rpc = (struct rpc_export *) relocate(exports->rpc_methods);
	for (i = 0; rpc && rpc[i].name; i++) {
	    parse_rpc(&res, rpc + i);
	    print_res(&res);
	}
    }

    if (print_all || print_params) {
	param = (struct param_export *) relocate(exports->params);
	for (i = 0; param && param[i].name; i++) {
	    parse_param(&res, param + i);
	    print_res(&res);
	}
    }

	if (print_all || print_selects) {
		s_table = lookup_sel_table(res.module);
		if (s_table) {
			select = get_module_selects(s_table);
			if (select) {
				scan_select_table(select);
			}
		}
	}

    exit(0);
}
