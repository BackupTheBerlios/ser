/*
 * $Id: snmp_mod.h,v 1.3 2002/09/19 12:23:55 jku Rel $
 *
 * SNMP Module
 * This is the main header for the snmp module 
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

#ifndef __SNMP_MOD_H_
#define __SNMP_MOD_H_ 

/* This SNMP headers don't play well with some of ser's #include's. 
 * Mix with care */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "../../dprint.h"
#include <errno.h>

/* stuff for dynamic handler registration */ 
#include "snmp_handler.h"

/* each header contains a part of our implemented MIB */
/* --- network services --- */
#include "applTable.h"
/* --- sipCommon --- */
#include "sipCommonCfgTable.h"
#include "sipCommonCfgBase.h"
#include "sipCommonCfgTimer.h"
#include "sipCommonCfgRetry.h"
#include "sipCommonCfgExpires.h"
#include "sipCommonStatsSummary.h"
#include "sipCommonStatsMethod.h"
#include "sipCommonStatusCode.h"
#include "sipCommonStatsTrans.h"
#include "sipCommonStatsRetry.h"
#include "sipCommonStatsOther.h"
/* --- sipServer --- */
#include "sipServerCfg.h"
#include "sipProxyCfg.h"
#include "sipProxyStats.h"
#include "sipRegCfg.h"
#include "sipRegStats.h"

/* similiar to TCP keep-alives. Useful if the snmpd agent goes down. Whenever
 * it decides to come back up, we'll contact it and re-register our MIBs */
#define SER_SNMP_PING 60

int ser_getApplIndex();

#endif
