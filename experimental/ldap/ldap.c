/*
 * LDAP module ldap
 *
 * Copyright (C) 2005 Arek Bekiersz
 * This module is released under the GNU General Publice License (GPL).
 *
 * Maintainer: Arek Bekiersz
 * Email:      arek@perceval.net
 *
 * Parts of the code copyright (C) 2004 Swiss Federal Institute of Technology (ETH Zurich)
 * Parts of the code copyright (C) 2004 Marcel Baur <baur@ethworld.ethz.ch>
 *
 *
 * Exported functions (some sort of manual):
 * ldap_does_uri_exist()                Checks if SIP account exists in LDAP (with SIP URI same as Request-URI)
 *
 *
 * ldap_proxy_authorize( char* realm )  Verifies credentials as in RFC2617. Checks Proxy-Authorization field.
 *                                      If no realm is specified (empty string), it is taken from request.
 *
 * ldap_www_authorize( char* realm )    Verifies credentials as in RFC2617. Checks Authorization field.
 *                                      If no realm is specified (empty string), it is taken from request.
 *
 * ldap_is_user_in( char* group )       Checks if SIP user is member of LDAP group (see NOTE 1)
 *
 *
 * ldap_lang_lookup()                   Checks 'preferredLanguage' attribute value of SIP account in LDAP 
 *                                      and rewrites Request-URI with ISO639 language code prepended in front
 *                                      (like "sip:en12345@domain.net")
 *
 * ldap_prefixes_lookup( char* mode )	Check if SIP account in LDAP has defined default Coutry and Local Area
 *                                      Code telephony prefixes. If yes, function prepends those prefixes in front
 *                                      of Request-URI.
 *                                      Example: Default country code: 18
 *                                               Default local area code: 43
 *                                               Dialed '5338462'. Called ldap_prefixes_lookup_f( "cclac" )
 *                                               Result is '0018435338462'
 *                                               Dialed '0435338462'. Called ldap_prefixes_lookup_f( "cc" )
 *                                               Result is '0018435338462'
 *
 * ldap_alias_lookup()                  Checks if there is SIP account in LDAP that has telephony alias
 *                                      same as current Request-URI. Aliases are stored in database in E.164 format.
 *                                      If such SIP account is found, function rewrites Request-URI with master
 *                                      SIP URI of this SIP account. Aliases must be unique throughout database
 *                                      (two different users cannot have the same alias).
 *                                      If calling user has default prefixes, they are prepended to dialed number
 *                                      so search is performed for aliased callees as well.
 *                                        
 *
 *
 *
 * NOTE 1: Function checks if special 'group' attribute of LDAP entry contains specified value
 *         Function should be rewritten to check LDAP group membership (groupOfUniqueNames)
 *
 *
 *
 */

/*
 * This file is an addition to ser, a free SIP server.
 * Copyright (C) 2001-2003 Fhg Fokus
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


#include "ldap.h"



/* ------------------------------------------------------------------	*/
/* Standard module initialization					*/
/* ------------------------------------------------------------------	*/

static int mod_init( void ) {
	int res;

	LOG(L_INFO, "LDAP module ldap.\nCopyright (C) 2005 Arek Bekiersz\n");
	LOG(L_INFO, "WARNING: This module is experimental and may crash SER\n");
	LOG(L_INFO, "or create unexpected results. You use the module at your\n");
	LOG(L_INFO, "own risk. Please submit bugs at http://bugs.sip-router.org/\n");

	res = bind_dbmod ();
	if (res < 0) {
		LOG(L_ERR, "ERROR: ldap - Unable to bind to db module, error = %i in mod_init()\n", res);
		return RESULT_NO;
	}

	db_con = db_init (db_url);
	if (db_con == NULL) {
		LOG(L_ERR, "ERROR: ldap - Error in db_init");
		return RESULT_NO;
	}

	pre_auth_func = (pre_auth_f)find_export("pre_auth", 0, 0);
	post_auth_func = (post_auth_f)find_export("post_auth", 0, 0);

	if (!(pre_auth_func && post_auth_func)) {
		LOG(L_ERR, "ERROR: ldap - This module requires auth module in mod_init()\n");
		return -2;
	}
  
	sl_reply = find_export("sl_send_reply", 2, 0);
	if (!sl_reply) {
		LOG(L_ERR, "ERROR: ldap - This module requires sl module in mod_init()\n");
		return -2;
	}

	LOG(L_INFO, "ldap: mod_init completed successfuly\n");
	return 0;
}

static int child_init( int rank ) {
	int bind;
	char* bug;

	/* LDAP database connection and binding */
	ld = ldap_init (LDAP_Server, LDAP_Port);

	if( ld == NULL) {
		LOG(L_ERR, "ERROR: ldap - Unable to contact server '%s:%i' in mod_init()\n", LDAP_Server, LDAP_Port);
		return RESULT_NO;
	}

	if( ((LDAP_Admin != NULL) && (LDAP_Admin != "") && (strlen(LDAP_Admin) > 0))
		&& ((LDAP_Passwd != NULL) && (LDAP_Passwd != "") && (strlen(LDAP_Passwd) > 0)) ) {

		bind = ldap_bind_s(ld, LDAP_Admin, LDAP_Passwd, LDAP_AUTH_SIMPLE);
		if( bind != LDAP_SUCCESS ) {
			bug = ldap_err2string(bind);
			LOG(L_ERR, "ERROR: ldap - Could not bind to database. Error code '%i - %s' in mod_init()\n", bind, bug);
			return RESULT_NO;
		}
	}
	return 0;
}

static void mod_destroy( void ) {

	if( ldap_unbind(ld) != LDAP_SUCCESS ) {
		LOG(L_ERR, "ERROR: ldap - could not unbind from database in mod_destroy(), continuing...\n");
	}

	LOG(L_INFO, "ldap: mod_destroy terminated successfuly");
}


/*	------------------------------------------------------------------- 	*/
/*	given a header field 'To: <sip:user@host>' lookup user@host in LDAP	*/
/*	and return RESULT_YES if found						*/
/*	-------------------------------------------------------------------	*/

static int ldap_does_uri_exist_f( struct sip_msg *mmm ) {

	/* struct to_body *receiver; */
	char *filter = NULL;
	str request_uri;
	int retval = RESULT_NO;

	/* receiver = get_to( mmm ); */
	if (get_request_uri(mmm, &request_uri) < 0) {
		LOG(L_ERR, "ERROR: ldap - Error while extracting Request-URI in ldap_does_uri_exist()\n");
		return retval;
	} 
	
	filter = get_filter( request_uri, LDAP_SIP_objectClass, LDAP_SIP_userAttrib );
	/* if( filter == "" ) { */
	if( filter == NULL ) {
		return retval;
	}
	
	if( search_filter( filter ) > 0 ) {
		retval = RESULT_YES;
	} else {
		/* nothing found */
		;
	}
	free( filter );
	return retval;
}

/*	------------------------------------------------------------------	*/
/*	Given URI and group name check if user is member of this group.		*/
/*	Return RESULT_YES, otherwise RESULT_NO.					*/
/*	------------------------------------------------------------------	*/

static int ldap_is_user_in_f( struct sip_msg* msg, char* hf, char* group) {

	int hf_type, retval;
	str* grp;
	str uri;
	
	grp = (str*)group;	/* via fixup */
	hf_type = (int)hf;

	switch(hf_type) {
		case 1: /* Request-URI */  
			if (get_request_uri(msg, &uri) < 0) {
				LOG(L_ERR, "ERROR: ldap - Error while extracting Request-URI in ldap_is_user_in()\n");
				return -1;
			} 
			break;

		case 2: /* To */
			if (get_to_uri(msg, &uri) < 0) {
				LOG(L_ERR, "ERROR: ldap - Error while extracting To in ldap_is_user_in()\n");
				return -2;
			}    
			break;

		case 3: /* From */
			if (get_from_uri(msg, &uri) < 0) {
				LOG(L_ERR, "ERROR: ldap - Error while extracting From in ldap_is_user_in()\n");
				return -3;
			}    
			break;
	}

	if( is_user_in_group( uri, *grp ) > 0 ) {
		retval = RESULT_YES;
	} else {
		retval = -4;
	}

	/* LOG(L_INFO, "ldap: ldap_is_user_in() returns '%i'", retval); */
	return retval;
}



static int ldap_alias_lookup_f( struct sip_msg* pMsg ) {

	str request_uri, from_uri;

	if (get_request_uri(pMsg, &request_uri) < 0) {
		LOG(L_ERR, "ERROR: ldap - Error while extracting Request-URI in ldap_alias_lookup()\n");
		return RESULT_NO;
	}    

	if (get_from_uri(pMsg, &from_uri) < 0) {
		LOG(L_ERR, "ERROR: ldap - Error while extracting From in ldap_alias_lookup()\n");
		return RESULT_NO;
	}

	if( rewrite_alias( request_uri, from_uri, pMsg) > 0 ) {
		return RESULT_YES;
	} else {
		return RESULT_NO;
	}
}

static int ldap_lang_lookup_f( struct sip_msg* pMsg) {

	str from_uri,request_uri;

	if (get_from_uri(pMsg, &from_uri) < 0) {
		LOG(L_ERR, "ERROR: ldap - Error while extracting From in ldap_lang_lookup()\n");
		return RESULT_NO;
	}

	if (get_request_uri(pMsg, &request_uri) < 0) {
		LOG(L_ERR, "ERROR: ldap - Error while extracting Request-URI in ldap_lang_lookup()\n");
		return RESULT_NO;
	}    

	if( rewrite_lang( from_uri, request_uri, pMsg) > 0 ) {
		return RESULT_YES;
	} else {
		return RESULT_NO;
	}
}

static int ldap_prefixes_lookup_f( struct sip_msg* pMsg, char* strMode) {

	str from_uri, request_uri;
	int intMode;

	if (get_from_uri(pMsg, &from_uri) < 0) {
		LOG(L_ERR, "ERROR: ldap - Error while extracting From in ldap_prefixes_lookup()\n");
		return RESULT_NO;
	}

	if (get_request_uri(pMsg, &request_uri) < 0) {
		LOG(L_ERR, "ERROR: ldap - Error while extracting Request-URI in ldap_prefixes_lookup()\n");
		return RESULT_NO;
	}    

	/* Choose mode of operation (Country Code or Country Code + Local Area Code) */
	if(  strncasecmp( strMode, "cclac", 5 ) == 0 ) {
		/* Country Code and Local Area Code */
		intMode = 4;
	} else if( strncasecmp( strMode, "cc", 2 ) == 0 ) {
		/* Country Code */
		intMode = 3;
	} else {
		/* Wrong parameter provided, exiting */
		LOG(L_ERR, "ERROR: ldap - Wrong parameter for ldap_prefixes_lookup()\n");
		return RESULT_NO;
	}

	if( rewrite_prefix( from_uri, request_uri, intMode, pMsg ) > 0 ) {
		return RESULT_YES;
	} else {
		return RESULT_NO;
	}
}

/*	------------------------------------------------------------------	*/
/*	Given message and domain parameter authorize user against LDAP entry	*/
/*	Return RESULT_YES, otherwise RESULT_NO.					*/
/*	------------------------------------------------------------------	*/

/* ---------------------------------------------
 * Authorize using Proxy-Authorize header field
 * ---------------------------------------------
 */
static int ldap_proxy_authorize_f(struct sip_msg* _m, char* _realm ) {
	/* realm parameter is converted to str* in str_fixup */
	/* LOG(L_INFO, "INFO: ldap - entered proxy_authorize()\n"); */
	return authorize(_m, (str*)_realm, HDR_PROXYAUTH);
}


/* -------------------------------------------
 * Authorize using WWW-Authorize header field
 * -------------------------------------------
 */
static int ldap_www_authorize_f(struct sip_msg* _m, char* _realm ) {
	/* LOG(L_INFO, "INFO: ldap - entered www_authorize()\n"); */
	return authorize(_m, (str*)_realm, HDR_AUTHORIZATION);
}
