/*
 * $Id: sipCommonStatsTrans.h,v 1.1 2002/08/28 19:02:59 ric Exp $
 *
 * SNMP Module
 * 
 * Note: this file originally auto-generated by mib2c using 
 * mib2c.sipdataset.conf 
 */
#ifndef SIPCOMMONSTATSTRANS_H
#define SIPCOMMONSTATSTRANS_H

/* function declarations */
int init_sipCommonStatsTrans(void);

reg_handler init_sipCurrentTransTable_h();
int sipCurrentTransTable_replaceRow(struct sip_snmp_obj *idx,
		struct sip_snmp_obj *data);

/* column number definitions for table sipCurrentTransTable */
#define COLUMN_SIPCURRENTTRANSACTIONS		1

#define SIPCURRENTTRANSTABLE_COLUMNS 1 /* number of columns */
#define SIPCURRENTTRANSTABLE_INDEXES 1	/* number of indexes */
reg_handler init_sipTransactionTable_h();
int sipTransactionTable_replaceRow(struct sip_snmp_obj *idx,
		struct sip_snmp_obj *data);

/* column number definitions for table sipTransactionTable */
#define COLUMN_SIPTRANSINDEX		1
#define COLUMN_SIPTRANSTO		2
#define COLUMN_SIPTRANSFROM		3
#define COLUMN_SIPTRANSCALLID		4
#define COLUMN_SIPTRANSCSEQ		5
#define COLUMN_SIPTRANSSTATE		6
#define COLUMN_SIPTRANSNUMOUTSTANDINGBRANCHES		7
#define COLUMN_SIPTRANSEXPIRY		8
#define COLUMN_SIPTRANSMETHOD		9
#define COLUMN_SIPTRANSACTIVITYINFO		10

#define SIPTRANSACTIONTABLE_COLUMNS 10 /* number of columns */
#define SIPTRANSACTIONTABLE_INDEXES 2	/* number of indexes */

#endif /* SIPCOMMONSTATSTRANS_H */
