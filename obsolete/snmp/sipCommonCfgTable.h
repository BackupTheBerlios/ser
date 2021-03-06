/*
 * $Id: sipCommonCfgTable.h,v 1.2 2002/09/19 12:23:54 jku Rel $
 *
 * SNMP Module
 * 
 * Note: this file originally auto-generated by mib2c using 
 * mib2c.sipdataset.conf 
 *
 * Copyright (C) 2001-2003 Fhg Fokus
 *
 * This file is part of ser, a free SIP server.
 *
 * ser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * For a license to use the ser software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact iptel.org by e-mail at the following addresses:
 *    info@iptel.org
 *
 * ser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SIPCOMMONCFGTABLE_H
#define SIPCOMMONCFGTABLE_H

/* function declarations */
int init_sipCommonCfgTable(void);

reg_handler init_sipCommonCfgTable_h();
int sipCommonCfgTable_replaceRow(struct sip_snmp_obj *idx,
		struct sip_snmp_obj *data);

int ser_add_sipCommonCfgTable();

/* column number definitions for table sipCommonCfgTable */
#define COLUMN_SIPPROTOCOLVERSION		1
#define COLUMN_SIPSERVICEOPERSTATUS		2
#define COLUMN_SIPSERVICEADMINSTATUS		3
#define COLUMN_SIPSERVICESTARTTIME		4
#define COLUMN_SIPSERVICELASTCHANGE		5
#define COLUMN_SIPORGANIZATION		6
#define COLUMN_SIPMAXSESSIONS		7
#define COLUMN_SIPREQUESTURIHOSTMATCHING		8

#define SIPCOMMONCFGTABLE_COLUMNS 8 /* number of columns */
#define SIPCOMMONCFGTABLE_INDEXES 1	/* number of indexes */

#endif /* SIPCOMMONCFGTABLE_H */
