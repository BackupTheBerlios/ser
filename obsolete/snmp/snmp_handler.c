/*
 * $Id: snmp_handler.c,v 1.1 2002/08/28 19:02:59 ric Exp $
 *
 * SNMP Module: The dynamic handler
 */
/*
 * The Idea: (applies directly to sipServer. For application is slightly
 * different, see below)
 * All of sipCommon's objects, have the prefix:
 * .1.3.6.1.2.1.9990.1
 * The different tables in sipCommon add the sufix #.#
 * For example, sipCommonCfgTable's OID is:
 * .1.3.6.1.2.1.9990.1.1.1
 * and sipSummaryStatsTable's is
 * .1.3.6.1.2.1.9990.1.5.1
 * Since the table's OID may differ only in those last 2 digits, 
 * the registration function uses them as indexes in the following tables to
 * find the function used to register new handlers with the tabled to which
 * the object belongs. The internal handler tables are indexed using the 
 * object's column number given by the last digit in the OID.
 * An example:
 * When registering the handler for sipProtocolVersion with OID
 * .1.3.6.1.2.1.9990.1.1.1.1.1
 * we take the pointer at sip_common_h[1][1] as the function used to
 * register a new handler into sipCommonCfgTable. Then, since sipProtocolVersion
 * is the first column in the table, we use 1 to call the sipCommonCfgTable
 * handler registration function:
 * sip_common_h[1][1](1, handler);
 * voila!
 *
 * For applicationMIB objects things are sort of simpler:
 * The prefix is .1.3.6.1.2.1.27, but tables only add an additional digit
 * to the OID, e.g. applTable is .1.3.6.1.2.1.27.1. We need one level less
 * of indirection
 */

#include "snmp_mod.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

/* The types. SNMP #includes conflict with ser #include's so here we
 * provide a mapping for ASN1 types */
const u_char ser_types[] = {
	ASN_INTEGER,	/* SER_INTEGER */ 
	ASN_INTEGER,	/* SER_INTEGER32 */ 
	ASN_UNSIGNED,	/* SER_UNSIGNED */ 
	ASN_TIMETICKS,	/* SER_TIMETICKS */
	ASN_GAUGE,		/* SER_GAUGE */ 
	ASN_COUNTER,	/* SER_COUNTER */ 
	ASN_INTEGER,	/* SER_ENUMVAL */ 
	ASN_OCTET_STR,	/* SER_STRING */ 
	ASN_OCTET_STR,	/* SER_BITSTRING */
	ASN_OBJECT_ID	/* SER_OBJID */
};

/* the big global handler tables. Indexed from 1. Stored in shared memory */
static reg_handler** sip_common_h;
static reg_handler** sip_server_h;
static reg_handler* application_h;

/***** the info for building the tables *****/
#define APPLICATION_IDX	2
#define SIPCOMMON_IDX1	10
static int sip_common_idx2[] = {
	0,4,2,2,2,1,2,3,2,2,1};
#define SIPSERVER_IDX1	8
static int sip_server_idx2[] = {
	0,1,0,1,1,0,0,3,1};

/* the funcs */
static int application_handler_init();
static int sipCommon_handler_init();
static int sipServer_handler_init();

/* these init funcs are all sort of static and if the SIP MIB 
 * changes they'll stop working. */
int handler_init()
{
	if(application_handler_init() == -1) {
		LOG(L_ERR, "snmp_mod: Couldn't initializer handler tables\n");
		return -1;
	}
	if(sipCommon_handler_init() == -1) {
		LOG(L_ERR, "snmp_mod: Couldn't initializer handler tables\n");
		return -1;
	}
	if(sipServer_handler_init() == -1) {
		LOG(L_ERR, "snmp_mod: Couldn't initializer handler tables\n");
		return -1;
	}
	return 0;
}


/* d -> destination, t->table, p->parent (for error message). e.g
 * init_table(application_h[1], applTable_h, "application")
 * These init functions initialize their internal handler table and return 
 * a function  which can be used to add a new handler to the table.
 */
#define init_table(d, t, p) { \
	d = init_##t();		\
	if(!d) {			\
		LOG(L_ERR, "snmp_mod: Error initializing " p " handler table\n"); \
		return -1;		\
	}					\
}

static int application_handler_init()
{
	application_h = calloc(APPLICATION_IDX+1, sizeof(reg_handler));
	if(!application_h) {
		LOG(L_ERR, "snmp_mod: Error initializing application handler table: "
				"Out of memory\n");
		return -1;
	}
	/* the handler tables are defined on their corresponding file */
	init_table(application_h[1], applTable_h, "application");
//	init_table(application_h[1], applTable_h, "application");

	return 0;
}

static int sipCommon_handler_init()
{
	int i;
	sip_common_h = calloc(SIPCOMMON_IDX1+1, sizeof(reg_handler*));
	if(!sip_common_h) {
		LOG(L_ERR, "snmp_mod: Error initializing sipCommon handler table: "
			"Out of memory\n");
		return -1;
	}
	for(i=1; i<SIPCOMMON_IDX1+1; i++) {
		sip_common_h[i] = calloc(sip_common_idx2[i]+1, sizeof(reg_handler));
		if(!sip_common_h[i]) {
			LOG(L_ERR, "snmp_mod: Error initializing sipCommon handler table: "
				"Out of memory\n");
			goto error;
		}
	}

	/* assign the real tables */
	init_table(sip_common_h[1][1], sipCommonCfgTable_h, "sipCommon");
	init_table(sip_common_h[1][2], sipPortTable_h, "sipCommon");
	init_table(sip_common_h[1][3], sipUriSupportedTable_h, "sipCommon");
	init_table(sip_common_h[1][4], sipFtrSupportedTable_h, "sipCommon"); 
	init_table(sip_common_h[2][1], sipCommonCfgTimerTable_h, "sipCommon"); 
	init_table(sip_common_h[2][2], sipCommonCfgTimerExtMethodTable_h, 
		"sipCommon"); 
	init_table(sip_common_h[3][1], sipCommonCfgRetryTable_h, "sipCommon"); 
	init_table(sip_common_h[3][2], sipCommonCfgRetryExtMethodTable_h, 
		"sipCommon"); 
	init_table(sip_common_h[4][1], sipCommonCfgExpiresMethodTable_h, 
		"sipCommon"); 
	init_table(sip_common_h[4][2], sipCommonCfgExpiresStatusCodeTable_h, 
		"sipCommon"); 
	init_table(sip_common_h[5][1], sipSummaryStatsTable_h, "sipCommon"); 
	init_table(sip_common_h[6][1], sipMethodStatsTable_h, "sipCommon"); 
	init_table(sip_common_h[6][2], sipStatsExtMethodTable_h, "sipCommon"); 
	init_table(sip_common_h[7][1], sipStatusCodesTable_h, "sipCommon");
	init_table(sip_common_h[7][2], sipCommonStatusCodeTable_h, "sipCommon");
	/* This is a trap table. We don't have handler code for this yet 
	init_table(sip_common_h[7][3], sipCommonStatusCodeNotifTable_h, "sipCommon");
	*/
	init_table(sip_common_h[8][1], sipCurrentTransTable_h, "sipCommon");
	init_table(sip_common_h[8][2], sipTransactionTable_h, "sipCommon");
	init_table(sip_common_h[9][1], sipCommonStatsRetryTable_h, "sipCommon"); 
	init_table(sip_common_h[9][2], sipCommonStatsRetryExtMethodTable_h, 
		"sipCommon");
	init_table(sip_common_h[10][1], sipOtherStatsTable_h, "sipCommon");

	return 0;
error:
	for(i--;i>0;i--)
		free(sip_common_h[i]);
	free(sip_common_h);
	return -1;
}

/* also includes the tables for SIP Registrars */
static int sipServer_handler_init()
{
	int i;
	sip_server_h = calloc(SIPSERVER_IDX1+1, sizeof(reg_handler*));
	if(!sip_server_h) {
		LOG(L_ERR, "snmp_mod: Error initializing sipServer handler table: "
			"Out of memory\n");
		return -1;
	}
	for(i=1; i<SIPSERVER_IDX1+1; i++) {
		sip_server_h[i] = calloc(sip_server_idx2[i]+1, sizeof(reg_handler));
		if(!sip_server_h[i]) {
			LOG(L_ERR, "snmp_mod: Error initializing sipServer handler table: "
				"Out of memory\n");
			goto error;
		}
	}
	/* assign the real tables */
	init_table(sip_server_h[1][1], sipServerCfgTable_h, "sipServer");
	init_table(sip_server_h[3][1], sipProxyCfgTable_h, "sipServer");
	init_table(sip_server_h[4][1], sipProxyStatsTable_h, "sipServer");
	init_table(sip_server_h[7][1], sipRegCfgTable_h, "sipServer");
	init_table(sip_server_h[7][2], sipRegUserTable_h, "sipServer");
	init_table(sip_server_h[7][3], sipContactTable_h, "sipServer");
	init_table(sip_server_h[8][1], sipRegStatsTable_h, "sipServer");

	return 0;
error:
	for(i--;i>0;i--)
		free(sip_server_h[i]);
	free(sip_server_h);
	return -1;
}

/* Info used to access the handler tables. Based on the MIB.
 * Change accordingly */
#define ID_POS			6
#define APPMIB_ID		27
#define SIPCOMMON_ID	9990
#define SIPSERVER_ID	9991
/* first is the position of the table in the OID, second the position
 * of the column. e.g.
 * sipUserUri -> .1.3.6.1.2.1.9991.1.7.2.1.2
 * table is sipRegUserTable (.7.2) at pos SIPSERVER_TABLE_POS (+1)
 * column is 2, at pos SIPSERVER_COL_POS
 */
#define APP_TABLE_POS	7
#define APP_COL_POS		9
#define SIPCOMMON_TABLE_POS	8
#define SIPCOMMON_COL_POS	11
#define SIPSERVER_TABLE_POS 8	/* ...just in case */
#define SIPSERVER_COL_POS	11
static int snmp_register(const char *name, struct sip_snmp_handler *h, int op)
{
	oid theOID[MAX_OID_LEN];
	size_t oid_len;
	int idx1, idx2;
	const char *func = "snmp_mod";

	/* get the OID for the name */
	oid_len = MAX_OID_LEN;
	memset(theOID, '\0', oid_len);
	/* this should be faster since no lookup is done, so we try it first */
	if(read_objid(name, theOID, &oid_len) == 0) {
		/* oh well, no luck, try again and see if name is something we
		 * can figure out... */
		oid_len = MAX_OID_LEN;
		if(get_node(name, theOID, &oid_len) == 0) {
			/* buuuuu... */
			LOG(L_ERR, "%s: Error getting OID for %s: %s\n", func, name,
					snmp_api_errstring(snmp_errno));
			return -1;
		}
	}

	/* figure out which table the object belongs to */
	switch(theOID[ID_POS]) {
		case APPMIB_ID:	/* here we use only one index */
			idx1 = theOID[APP_TABLE_POS];
			if(!application_h[idx1]) {
				LOG(L_WARN, "%s: Don't have a registration function for %s\n",
					func, name);
				return -1;
			}
			if(op == REG_OBJ)
				h->sip_obj->col = theOID[APP_COL_POS];
			if(application_h[idx1](h, op) == -1) {
				LOG(L_ERR, "%s: Error registering handler\n", func);
				return -1;
			}
			break;
		case SIPCOMMON_ID:
			idx1 = theOID[SIPCOMMON_TABLE_POS];
			idx2 = theOID[SIPCOMMON_TABLE_POS+1];
			if(!sip_common_h[idx1][idx2]) {
				LOG(L_WARN, "%s: Don't have a registration function for %s\n",
					func, name);
				return -1;
			}
			if(op == REG_OBJ)
				h->sip_obj->col = theOID[SIPCOMMON_COL_POS];
			if(sip_common_h[idx1][idx2](h, op) == -1) {
				LOG(L_ERR, "%s: Error registering handler\n", func);
				return -1;
			}
			break;
		case SIPSERVER_ID:
			idx1 = theOID[SIPSERVER_TABLE_POS];
			idx2 = theOID[SIPSERVER_TABLE_POS+1];
			/* chk if there's a registration function for this table.. */
			if(!sip_server_h[idx1][idx2]) {
				LOG(L_WARN, "%s: Don't have a registration function for %s\n",
					func, name);
				return -1;
			}
			if(op == REG_OBJ)
				h->sip_obj->col = theOID[SIPSERVER_COL_POS];
			/* ... call it */
			if(sip_server_h[idx1][idx2](h, op) == -1) {
				LOG(L_ERR, "%s: Error registering handler\n", func);
				return -1;
			}
			break;
	}

	return 0;
}

/* 
 * Registers a handler for a particular column in a table. 
 * There's no need to pass the column number in the handler structure. 
 * For most of the SIP MIB tables, each handler is effectively a handler 
 * for a particular SNMP object, since there's only one row in the table.
 * Nonetheless, the handling function will be passed the row that it 
 * should process. For the same reason, we don't support having handlers
 * for different rows.
 * Along the same lines, note that the data (value) passed here will
 * always be added to the same row. If you pass a handler for the same
 * column twice, no complains will be made and your previous data will be
 * silently replaced. If you need to create more than
 * one row for your table use snmp_register_row() (see below).
 * A copy of the handler is made internally, but not of its value
 * (h->sip_obj->value). You can free all memory used by the handler,
 * included the value upon return from this function.
 * If an error occurs, -1 is returned. 
 */
int snmp_register_handler(const char *name, struct sip_snmp_handler *h)
{
	const char *func = "snmp_mod";

	if(!h || !h->sip_obj || !h->sip_obj->value.voidp) {
		LOG(L_ERR, "%s: Invalid handler to register\n", func);
		return -1;
	}

	if(snmp_register(name, h, REG_OBJ) == -1) {
		LOG(L_ERR, "%s: Error registering handler for %s\n", func, name);
		return -1;
	}

	return 0;
}

/* Creates a new row in table table. The first time its called, the passed
 * functions are also registered as the handlers for their particular
 * column. It's not a good idea to combine calls from here with
 * calls to snmp_register_handler() since for now they don't try
 * to "cooperate" (it would make all the code more complicated and i can't
 * find a good reason to have them cooperate). 
 * Each table knows how many indexes it has and which of them should be also
 * added to the table as objects. However, the handlers should be
 * in correct order, starting with the index(es) for the new row. Note
 * that if applIndex is an index in your table you don't need to pass
 * it through here, it will be added automatically. If applIndex is the
 * only index in your table, then start the handler list with the first 
 * object in your row.
 * Returns -1 on error, 0 on success.
 */
int snmp_register_row(const char *table, struct sip_snmp_handler *h)
{
	const char *func = "snmp_mod";

	if(!h || !h->sip_obj || !h->sip_obj->value.voidp) {
		LOG(L_ERR, "%s: Invalid handler to register\n", func);
		return -1;
	}

	if(snmp_register(table, h, NEW_ROW) == -1) {
		LOG(L_ERR, "%s: Error creating new row in %s\n", func, table);
		return -1;
	}

	return 0;
}

/* registers a handler for a complete table. After registering this 
 * handler you need to create rows with appropiate index(es) using 
 * snmp_register_row */
int snmp_register_table(const char *name, struct sip_snmp_handler *h)
{
	const char *func = "snmp_mod";

	if(!h) {
		LOG(L_ERR, "%s: Invalid handler for table %s\n", func, name);
		return -1;
	}
	
	if(snmp_register(name, h, REG_TABLE) == -1) {
		LOG(L_ERR, "%s: Error registering handler table for %s\n", func, name);
		return -1;
	}

	return 0;
}

struct sip_snmp_handler* snmp_new_handler(size_t val_len)
{
	struct sip_snmp_handler *h;

	h = calloc(1, sizeof(struct sip_snmp_handler));
	if(!h)
		return NULL;

	h->sip_obj = calloc(1, sizeof(struct sip_snmp_obj));
	if(!h->sip_obj) {
		free(h);
		return NULL;
	}
	if(val_len > 0) {
		h->sip_obj->value.voidp = calloc(1, val_len);
		if(!h->sip_obj->value.voidp) {
			free(h->sip_obj);
			free(h);
			return NULL;
		}
		h->sip_obj->val_len = val_len;
	}

	return h;
}


void snmp_free_handler(struct sip_snmp_handler *h)
{
	if(h->sip_obj->value.voidp)
		free(h->sip_obj->value.voidp);
	free(h->sip_obj);
	free(h);
}

/* clones h and h->sip_obj unless one of them is NULL. we don't need
 * the value so we don't clone it */
struct sip_snmp_handler* snmp_clone_handler(struct sip_snmp_handler* h)
{
	struct sip_snmp_handler *i;
	if(!h)
		return NULL;

	i = calloc(1, sizeof(struct sip_snmp_handler));
	if(!i)
		return NULL;
	memcpy(i, h, sizeof(struct sip_snmp_handler));

	if(!h->sip_obj)
		return i;

	i->sip_obj = calloc(1, sizeof(struct sip_snmp_obj));
	if(!i->sip_obj) {
		free(i);
		return NULL;
	}
	memcpy(i->sip_obj, h->sip_obj, sizeof(struct sip_snmp_obj));

	i->sip_obj->value.voidp = NULL;

	return i;
}

struct sip_snmp_obj* snmp_new_obj(enum ser_types type, void *value,
		size_t val_len)
{
	struct sip_snmp_obj *o;
	o = calloc(1, sizeof(struct sip_snmp_obj));
	if(!o)
		return NULL;
	o->type = type;
	o->val_len = val_len;
	if(val_len <= 0)
		return o;

	o->value.voidp = calloc(1, val_len);
	if(!o->value.voidp) {
		free(o);
		return NULL;
	}
	memcpy(o->value.voidp, value, val_len);

	return o;
}

void snmp_free_obj(struct sip_snmp_obj *o)
{
	if(o->value.voidp)
		free(o->value.voidp);
	free(o);
}
