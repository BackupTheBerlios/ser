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

#include "osp_mod.h"
#include "term_transaction.h"
#include "sipheader.h"
#include "destination.h"
#include "osptoolkit.h"
#include "usage.h"

extern char* _osp_device_ip;
extern int _osp_token_format;
extern int _osp_validate_callid;
extern OSPTPROVHANDLE _osp_provider;

/*
 * Get OSP token
 * param msg SIP message
 * param ignore1
 * param ignore2
 * return  MODULE_RETURNCODE_TRUE success, MODULE_RETURNCODE_FALSE failure
 */
int ospCheckHeader(
    struct sip_msg* msg, 
    char* ignore1, 
    char* ignore2)
{
    unsigned char buffer[OSP_TOKENBUF_SIZE];
    unsigned int  buffersize = sizeof(buffer);

    LOG(L_DBG, "osp: ospCheckHeader\n");

    if (ospGetOspHeader(msg, buffer, &buffersize) != 0) {
        return MODULE_RETURNCODE_FALSE;
    } else {
        return MODULE_RETURNCODE_TRUE;
    }
}

/*
 * Validate OSP token
 * param ignore1
 * param ignore2
 * return  MODULE_RETURNCODE_TRUE success, MODULE_RETURNCODE_FALSE failure
 */
int ospValidateHeader (
    struct sip_msg* msg, 
    char* ignore1, 
    char* ignore2)
{
    int errorcode; 
    OSPTTRANHANDLE transaction = -1;
    unsigned int authorized = 0;
    unsigned int timelimit = 0;
    void* detaillog = NULL;
    unsigned int logsize = 0;
    unsigned char* callidval = (unsigned char*)"";
    OSPTCALLID* callid = NULL;
    unsigned callidsize = 0;
    unsigned char token[OSP_TOKENBUF_SIZE];
    unsigned int tokensize = sizeof(token);
    osp_dest dest;
    int result = MODULE_RETURNCODE_FALSE;

    LOG(L_DBG, "osp: ospValidateHeader\n");

    ospInitDestination(&dest);

    if ((errorcode = OSPPTransactionNew(_osp_provider, &transaction) != 0)) {
        LOG(L_ERR, "osp: ERROR: failed to create a new OSP transaction handle (%d)\n", errorcode);
    } else if ((ospGetRpidUserpart(msg, dest.calling, sizeof(dest.calling)) != 0) && 
        (ospGetFromUserpart(msg, dest.calling, sizeof(dest.calling)) != 0))
    {
        LOG(L_ERR, "osp: ERROR: failed to extract calling number\n");
    } else if ((ospGetUriUserpart(msg, dest.called, sizeof(dest.called)) != 0) &&
        (ospGetToUserpart(msg, dest.called, sizeof(dest.called)) != 0))
    {
        LOG(L_ERR, "osp: ERROR: failed to extract called number\n");
    } else if (ospGetCallId(msg, &callid) != 0) {
        LOG(L_ERR, "osp: ERROR: failed to extract call id\n");
    } else if (ospGetSourceAddress(msg, dest.source, sizeof(dest.source)) != 0) {
        LOG(L_ERR, "osp: ERROR: failed to extract source address\n");
    } else if (ospGetOspHeader(msg, token, &tokensize) != 0) {
        LOG(L_ERR, "osp: ERROR: failed to extract OSP authorization token\n");
    } else {
        LOG(L_INFO, 
            "osp: validate token for: "
            "transaction_handle '%i' "
            "e164_source '%s' "
            "e164_dest '%s' "
            "validate_call_id '%s' "
            "call_id '%.*s'\n",
            transaction,
            dest.calling,
            dest.called,
            _osp_validate_callid == 0 ? "No" : "Yes",
            callid->ospmCallIdLen,
            callid->ospmCallIdVal);

        if (_osp_validate_callid != 0) {
            callidsize = callid->ospmCallIdLen;
            callidval = callid->ospmCallIdVal;
        }

        errorcode = OSPPTransactionValidateAuthorisation(
            transaction,
            "",
            "",
            "",
            "",
            dest.calling,
            OSPC_E164,
            dest.called,
            OSPC_E164,
            callidsize,
            callidval,
            tokensize,
            token,
            &authorized,
            &timelimit,
            &logsize,
            detaillog,
            _osp_token_format);
    
        if (callid->ospmCallIdLen > sizeof(dest.callid) - 1) {
            dest.callidsize = sizeof(dest.callid) - 1;
        } else {
            dest.callidsize = callid->ospmCallIdLen;
        }
        memcpy(dest.callid, callid->ospmCallIdVal, dest.callidsize);
        dest.callid[dest.callidsize] = 0;
        dest.tid = ospGetTransactionId(transaction);
        dest.type = OSPC_DESTINATION;
        dest.authtime = time(NULL);
        strcpy(dest.host, _osp_device_ip);

        ospSaveTermDestination(&dest);

        if ((errorcode == 0) && (authorized == 1)) {
            LOG(L_DBG, 
                "osp: call is authorized for %d seconds, call_id '%.*s' transaction_id '%lld'",
                timelimit,
                dest.callidsize,
                dest.callid,
                dest.tid);
            ospRecordTermTransaction(msg, transaction, dest.source, dest.calling, dest.called, dest.authtime);
            result = MODULE_RETURNCODE_TRUE;
        } else {
            LOG(L_ERR, "osp: ERROR: token is invalid (%i)\n", errorcode);

            /* 
             * Update terminating status code to 401 and report terminating setup usage.
             * We may need to make 401 configurable, just in case a user decides to reply with
             * a different code.  Other options - trigger call setup usage reporting from the cpl
             * (after replying with an error code), or maybe use a different tm callback.
             */
            ospRecordEvent(0, 401);
        }
    }

    if (transaction != -1) {
        OSPPTransactionDelete(transaction);
    }

    if (callid != NULL) {
        OSPPCallIdDelete(&callid);
    }
    
    return result;
}

