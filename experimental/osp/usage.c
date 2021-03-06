/*
 * ser osp module. 
 *
 * This module enables ser to communicate with an Open Settlement 
 * Protocol (OSP) server.  The Open Settlement Protocol is an ETSI 
 * defined standard for Inter-Domain VoIP pricing, authorization
 * and usage exchange.  The technical specifications for OSP 
 * (ETSI TS 101 321 V4.1.1) are available at www.etsi.org.
 *
 * Uli Abend was the original contributor to this module.
 * 
 * Copyright (C) 2001-2005 Fhg Fokus
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

#include "../../sr_module.h"
#include "../../usr_avp.h"
#include "usage.h"
#include "destination.h"
#include "osptoolkit.h"
#include "sipheader.h"

#define OSP_ORIG_COOKIE     "osp-o"
#define OSP_TERM_COOKIE     "osp-t"

#define OSP_RELEASE_ORIG    0
#define OSP_RELEASE_TERM    1

/* SER uses AVP to add RR pararmters */
const str OSP_ORIGCOOKIE_NAME = {"_osp_orig_cookie_", 17};
const str OSP_TERMCOOKIE_NAME = {"_osp_term_cookie_", 17};

extern char* _osp_device_ip;
extern OSPTPROVHANDLE _osp_provider;
extern str OSP_ORIGDEST_NAME;

static void ospRecordTransaction(struct sip_msg* msg, OSPTTRANHANDLE transaction, char* uac, char* from, char* to, time_t authtime, int isorig);
static int ospBuildUsageFromDestination(OSPTTRANHANDLE transaction, osp_dest* dest, int lastcode);
static int ospReportUsageFromDestination(OSPTTRANHANDLE transaction, osp_dest* dest);
/* SER checks ftag by itself, without release parameter */
static int ospReportUsageFromCookie(struct sip_msg* msg, char* cooky, OSPTCALLID* callid, int isorig);

/*
 * Create OSP cookie and insert it into Record-Route header
 * param msg SIP message
 * param tansaction Transaction handle
 * param uac Source IP
 * param from
 * param to
 * param authtime Request authorization time
 * param isorig Originate / Terminate
 */
static void ospRecordTransaction(
    struct sip_msg* msg, 
    OSPTTRANHANDLE transaction, 
    char* uac, 
    char* from, 
    char* to, 
    time_t authtime, 
    int isorig)
{
    const str* name;
    str cookie;
    char buffer[OSP_STRBUF_SIZE];

    LOG(L_DBG, "osp: ospRecordTransaction\n");

    cookie.s = buffer;
    cookie.len = snprintf(
        buffer,
        sizeof(buffer),
        "t%llu_s%s_T%d",
        ospGetTransactionId(transaction),
        uac,
        (unsigned int)authtime);
    if (cookie.len < 0) {
        LOG(L_ERR, "osp: ERROR: failed to create OSP cookie\n");
        return;
    }

    /* SER uses AVP to add RR parameters */
    LOG(L_DBG, "osp: adding RR parameter '%s' for '%s'\n", 
        buffer, 
        (isorig == 1) ? "orig" : "term");
    name = (isorig == 1) ? &OSP_ORIGCOOKIE_NAME : &OSP_TERMCOOKIE_NAME;
    add_avp(AVP_NAME_STR | AVP_VAL_STR, (int_str)*name, (int_str)cookie);
}

/*
 * Create OSP originate cookie and insert it into Record-Route header
 * param msg SIP message
 * param tansaction Transaction handle
 * param uac Source IP
 * param from
 * param to
 * param authtime Request authorization time
 */
void ospRecordOrigTransaction(
    struct sip_msg* msg, 
    OSPTTRANHANDLE transaction, 
    char* uac, 
    char* from, 
    char* to, 
    time_t authtime)
{
    int isorig = 1;

    LOG(L_DBG, "osp: ospRecordOrigTransaction\n");

    ospRecordTransaction(msg, transaction, uac, from, to, authtime, isorig);
}

/*
 * Create OSP terminate cookie and insert it into Record-Route header
 * param msg SIP message
 * param tansaction Transaction handle
 * param uac Source IP
 * param from
 * param to
 * param authtime Request authorization time
 */
void ospRecordTermTransaction(
    struct sip_msg* msg, 
    OSPTTRANHANDLE transaction, 
    char* uac, 
    char* from, 
    char* to, 
    time_t authtime)
{
    int isorig = 0;

    LOG(L_DBG, "osp: ospRecordTermTransaction\n");

    ospRecordTransaction(msg, transaction, uac, from, to, authtime, isorig);
}

/*
 * Report OSP usage from OSP cookie
 *     SER checks ftag by itself, without release parameter
 * param msg SIP message
 * param cookie OSP cookie
 * param callid Call ID
 * param isorig Originate / Terminate
 * return
 */
static int ospReportUsageFromCookie(
    struct sip_msg* msg,
    char* cookie, 
    OSPTCALLID* callid, 
    int isorig) 
{
    int release;
    char* tmp;
    char* token;
    char tag;
    char* value;
    unsigned long long transactionid = 0;
    char* uac = "";
    time_t authtime = -1;
    time_t endtime = time(NULL);
    char firstvia[OSP_STRBUF_SIZE];
    char from[OSP_STRBUF_SIZE];
    char to[OSP_STRBUF_SIZE];
    char nexthop[OSP_STRBUF_SIZE];
    char* calling;
    char* called;
    char* terminator;
    unsigned issource;
    char* source;
    char srcbuf[OSP_STRBUF_SIZE];
    char* destination;
    char dstbuf[OSP_STRBUF_SIZE];
    char* srcdev;
    char devbuf[OSP_STRBUF_SIZE];
    OSPTTRANHANDLE transaction = -1;
    int errorcode;

    LOG(L_DBG, "osp: ospReportUsageFromCookie\n");

    LOG(L_DBG, "osp: '%s' isorig '%d'\n", cookie, isorig);
    for (token = strtok_r(cookie, "_", &tmp);
        token;
        token = strtok_r(NULL, "_", &tmp))
    {
        tag = *token;
        value= token + 1;

        switch (tag) {
            case 't':
                transactionid = atoll(value);
                break;
            case 'T':
                authtime = atoi(value);
                break;
            case 's':
                uac = value;
                break;
            default:
                LOG(L_ERR, "osp: ERROR: unexpected tag '%c' / value '%s'\n", tag, value);
                break;
        }
    }

    ospGetSourceAddress(msg, firstvia, sizeof(firstvia));
    ospGetFromUserpart(msg, from, sizeof(from));
    ospGetToUserpart(msg, to, sizeof(to));
    ospGetNextHop(msg, nexthop, sizeof(nexthop));

    LOG(L_DBG, "osp: first via '%s' from '%s' to '%s' next hop '%s'\n",
        firstvia,
        from,
        to,
        nexthop);

    /* SER checks ftag by itself */
    errorcode = ospGetDirection(msg);
    switch (errorcode) {
        case 0:
            release = OSP_RELEASE_ORIG;
            break;
        case 1:
            release = OSP_RELEASE_TERM;
            break;
        default:
            /* This approach has a problem of flipping called/calling number */
            if (strcmp(firstvia, uac) == 0) {
                release = OSP_RELEASE_ORIG;
            } else {
                release = OSP_RELEASE_TERM;
            }
    }

    if (release == OSP_RELEASE_ORIG) {
        LOG(L_DBG,
            "osp: orig '%s' released the call, call_id '%.*s' transaction_id '%lld'\n",
            firstvia,
            callid->ospmCallIdLen,
            callid->ospmCallIdVal,
            transactionid);
        calling = from;
        called = to;
        terminator = nexthop;
    } else {
        release = OSP_RELEASE_TERM;
        LOG(L_DBG,
            "osp: term '%s' released the call, call_id '%.*s' transaction_id '%lld'\n",
            firstvia,
            callid->ospmCallIdLen,
            callid->ospmCallIdVal,
            transactionid);
        calling = to;
        called = from;
        terminator = firstvia;
    }

    errorcode = OSPPTransactionNew(_osp_provider, &transaction);

    LOG(L_DBG, "osp: created transaction handle '%d' (%d)\n", transaction, errorcode);

    if (isorig == 1) {
        issource = OSPC_SOURCE;
        source = _osp_device_ip;
        ospConvertAddress(terminator, dstbuf, sizeof(dstbuf));
        destination = dstbuf;
        ospConvertAddress(uac, devbuf, sizeof(devbuf));
        srcdev = devbuf;
    } else {
        issource = OSPC_DESTINATION;
        ospConvertAddress(uac, srcbuf, sizeof(srcbuf));
        source = srcbuf;
        destination = _osp_device_ip;
        srcdev = "";
    }

    errorcode = OSPPTransactionBuildUsageFromScratch(
        transaction,
        transactionid,
        issource,
        source,
        destination,
        srcdev,
        "",
        calling,
        OSPC_E164,
        called,
        OSPC_E164,
        callid->ospmCallIdLen,
        callid->ospmCallIdVal,
        (enum OSPEFAILREASON)0,
        NULL,
        NULL);

    LOG(L_DBG, "osp: built usage handle '%d' (%d)\n", transaction, errorcode);

    ospReportUsageWrapper(
        transaction,
        10016,
        endtime - authtime,
        authtime,
        endtime,
        0,0,
        0,0,
        release);

    return errorcode;
}

/*
 * Report OSP usage
 *     SER uses AVP to add RR pararmters
 * param msg SIP message
 * param ignore1
 * param ignore2
 * return MODULE_RETURNCODE_TRUE success, MODULE_RETURNCODE_FALSE failure
 */
int ospReportUsage(
    struct sip_msg* msg, 
    char* ignore1,
    char* ignore2)
{
    int_str cookieval;
    struct usr_avp* cookieavp = NULL;
    static const int FROMFLAGS = AVP_TRACK_FROM | AVP_CLASS_URI | AVP_NAME_STR | AVP_VAL_STR;
    static const int TOFLAGS = AVP_TRACK_TO | AVP_CLASS_URI | AVP_NAME_STR | AVP_VAL_STR;
    char buffer[OSP_HEADERBUF_SIZE];
    int isorig;
    OSPTCALLID* callid = NULL;
    int result = MODULE_RETURNCODE_FALSE;

    LOG(L_DBG, "osp: ospReportUsage\n");

    ospGetCallId(msg, &callid);
    if (callid != NULL) {
        if ((cookieavp = search_first_avp(FROMFLAGS, (int_str)OSP_ORIGCOOKIE_NAME, &cookieval, NULL)) != 0 ||
            (cookieavp = search_first_avp(TOFLAGS, (int_str)OSP_ORIGCOOKIE_NAME, &cookieval, NULL)) != 0)
        {
            ospCopyStrToBuffer(&cookieval.s, buffer, sizeof(buffer));
            LOG(L_DBG, "orig cookie '%s'\n", buffer);
            LOG(L_INFO,
                "osp: report orig duration for call_id '%.*s'\n",
                callid->ospmCallIdLen,
                callid->ospmCallIdVal);
            isorig = 1;
            ospReportUsageFromCookie(msg, buffer, callid, isorig);
            result = MODULE_RETURNCODE_TRUE;
        }

        if ((cookieavp = search_first_avp(FROMFLAGS, (int_str)OSP_TERMCOOKIE_NAME, &cookieval, NULL)) != 0 ||
            (cookieavp = search_first_avp(TOFLAGS, (int_str)OSP_TERMCOOKIE_NAME, &cookieval, NULL)) != 0)
        {
            ospCopyStrToBuffer(&cookieval.s, buffer, sizeof(buffer));
            LOG(L_DBG, "term cookie '%s'\n", buffer);
            LOG(L_INFO,
                "osp: report term duration for call_id '%.*s'\n",
                callid->ospmCallIdLen,
                callid->ospmCallIdVal);
            isorig = 0;
            ospReportUsageFromCookie(msg, buffer, callid, isorig);
            result = MODULE_RETURNCODE_TRUE;
        }
        OSPPCallIdDelete(&callid);
    }

    if (result == MODULE_RETURNCODE_FALSE) {
        LOG(L_DBG, "osp: without OSP orig or term usage information\n");
    }

    return result;
}

/*
 * Build OSP usage from destination
 * param transaction OSP transaction handle
 * param dest Destination
 * param lastcode Destination status
 * return 0 success, others failure
 */
static int ospBuildUsageFromDestination(
    OSPTTRANHANDLE transaction, 
    osp_dest* dest, 
    int lastcode)
{
    int errorcode;
    char addr[OSP_STRBUF_SIZE];
    char* source;
    char* srcdev;

    dest->reported = 1;

    LOG(L_DBG, "osp: ospBuildUsageFromDestination\n");

    if (dest->type == OSPC_SOURCE) {
        ospConvertAddress(dest->srcdev, addr, sizeof(addr));
        source = dest->source;
        srcdev = addr;
    } else {
        ospConvertAddress(dest->source, addr, sizeof(addr));
        source = addr;
        srcdev = dest->srcdev;
    }

    errorcode = OSPPTransactionBuildUsageFromScratch(
        transaction,
        dest->tid,
        dest->type,
        source,
        dest->host,
        srcdev,
        dest->destdev,
        dest->calling,
        OSPC_E164,
        dest->called,
        OSPC_E164,
        dest->callidsize,
        dest->callid,
        (enum OSPEFAILREASON)lastcode,
        NULL,
        NULL);

    return errorcode;
}

/*
 * Report OSP usage from destination
 * param transaction OSP transaction handle
 * param dest Destination
 * return 0 success
 */
static int ospReportUsageFromDestination(
    OSPTTRANHANDLE transaction, 
    osp_dest* dest)
{
    ospReportUsageWrapper(
        transaction,                                          /* In - Transaction handle */
        dest->lastcode,                                       /* In - Release Code */    
        0,                                                    /* In - Length of call */
        dest->authtime,                                       /* In - Call start time */
        0,                                                    /* In - Call end time */
        dest->time180,                                        /* In - Call alert time */
        dest->time200,                                        /* In - Call connect time */
        dest->time180 ? 1 : 0,                                /* In - Is PDD Info present */
        dest->time180 ? dest->time180 - dest->authtime : 0,   /* In - Post Dial Delay */
        0);

    return 0;
}

/*
 * Report originate call setup usage
 */
void ospReportOrigSetupUsage(void)
{
    osp_dest* dest = NULL;
    osp_dest* lastused = NULL;
    struct usr_avp* destavp = NULL;
    int_str destval;
    struct search_state state;
    OSPTTRANHANDLE transaction = -1;
    int lastcode = 0;
    int errorcode = 0;

    LOG(L_DBG, "osp: ospReportOrigSetupUsage\n");

    errorcode = OSPPTransactionNew(_osp_provider, &transaction);

    for (destavp = search_first_avp(AVP_NAME_STR | AVP_VAL_STR, (int_str)OSP_ORIGDEST_NAME, &destval, &state);
        destavp != NULL;
        destavp = search_next_avp(&state, &destval)) 
    {
        /* OSP destination is wrapped in a string */
        dest = (osp_dest*)destval.s.s;

        if (dest->used == 1) {
            if (dest->reported == 1) {
                LOG(L_DBG, "osp: orig setup already reported\n");
                break;
            }

            LOG(L_DBG, "osp: iterating through used destination\n");

            ospDumpDestination(dest);

            lastused = dest;

            errorcode = ospBuildUsageFromDestination(transaction, dest, lastcode);

            lastcode = dest->lastcode;
        } else {
            LOG(L_DBG, "osp: destination has not been used, breaking out\n");
            break;
        }
    }

    if (lastused) {
        LOG(L_INFO,
            "osp: report orig setup for call_id '%.*s' transaction_id '%lld'\n",
            lastused->callidsize,
            lastused->callid,
            lastused->tid);
        errorcode = ospReportUsageFromDestination(transaction, lastused);
    } else {
        /* If a Toolkit transaction handle was created, but we did not find
         * any destinations to report, we need to release the handle. Otherwise,
         * the ospReportUsageFromDestination will release it.
         */
        OSPPTransactionDelete(transaction);
    }
}

/*
 * Report terminate call setup usage
 */
void ospReportTermSetupUsage(void)
{
    osp_dest* dest = NULL;
    OSPTTRANHANDLE transaction = -1;
    int errorcode = 0;

    LOG(L_DBG, "osp: ospReportTermSetupUsage\n");

    if ((dest = ospGetTermDestination())) {
        if (dest->reported == 0) {
            LOG(L_INFO,
                "osp: report term setup for call_id '%.*s' transaction_id '%lld'\n",
                dest->callidsize,
                dest->callid,
                dest->tid);
            errorcode = OSPPTransactionNew(_osp_provider, &transaction);
            errorcode = ospBuildUsageFromDestination(transaction, dest, 0);
            errorcode = ospReportUsageFromDestination(transaction, dest);
        } else {
            LOG(L_DBG, "osp: term setup already reported\n");
        }
    } else {
        LOG(L_ERR, "osp: ERROR: without term setup to report\n");
    }
}
