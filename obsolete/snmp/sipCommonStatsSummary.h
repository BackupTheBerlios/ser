/*
 * $Id: sipCommonStatsSummary.h,v 1.2 2002/09/19 12:23:55 jku Rel $
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

#ifndef SIPCOMMONSTATSSUMMARY_H
#define SIPCOMMONSTATSSUMMARY_H

/* function declarations */
int init_sipCommonStatsSummary(void);

reg_handler init_sipSummaryStatsTable_h();
int sipSummaryStatsTable_replaceRow(struct sip_snmp_obj *idx,
		struct sip_snmp_obj *data);

/* column number definitions for table sipSummaryStatsTable */
#define COLUMN_SIPSUMMARYINREQUESTS		1
#define COLUMN_SIPSUMMARYOUTREQUESTS		2
#define COLUMN_SIPSUMMARYINRESPONSES		3
#define COLUMN_SIPSUMMARYOUTRESPONSES		4
#define COLUMN_SIPSUMMARYTOTALTRANSACTIONS		5

#define SIPSUMMARYSTATSTABLE_COLUMNS 5 /* number of columns */
#define SIPSUMMARYSTATSTABLE_INDEXES 1	/* number of indexes */

#endif /* SIPCOMMONSTATSSUMMARY_H */
