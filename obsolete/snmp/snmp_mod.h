/*
 * * $Id $
 *
 * SNMP Module
 * This is the main header for the snmp module 
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
