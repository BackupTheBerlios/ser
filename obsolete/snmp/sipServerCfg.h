/*
 * $Id: sipServerCfg.h,v 1.1 2002/08/28 19:02:59 ric Exp $
 *
 * SNMP Module
 * 
 * Note: this file originally auto-generated by mib2c using 
 * mib2c.sipdataset.conf 
 */
#ifndef SIPSERVERCFG_H
#define SIPSERVERCFG_H

/* function declarations */
int init_sipServerCfg(void);

reg_handler init_sipServerCfgTable_h();
int sipServerCfgTable_replaceRow(struct sip_snmp_obj *idx,
		struct sip_snmp_obj *data);

/* column number definitions for table sipServerCfgTable */
#define COLUMN_SIPSERVERHOSTADDRTYPE		1
#define COLUMN_SIPSERVERHOSTADDR		2
#define COLUMN_SIPPGPVERSION		3
#define COLUMN_SIPSERVERCONTACTDFLTACTION		4
#define COLUMN_SIPSERVERRESPECTUAACTION		5

#define SIPSERVERCFGTABLE_COLUMNS 5 /* number of columns */
#define SIPSERVERCFGTABLE_INDEXES 1	/* number of indexes */

#endif /* SIPSERVERCFG_H */
