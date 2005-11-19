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
 * Please see ldap.c for details
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





/* ---- Includes --------------------- */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ldap.h>

#include "../../sr_module.h"
#include "../../action.h"
#include "../../db/db.h"
#include "../../parser/parse_to.h"
#include "../../parser/parse_uri.h"
#include "../../parser/parse_from.h"
#include "../../parser/hf.h"
#include "../../parser/digest/digest.h"
#include "../../mem/mem.h"
#include "../../ut.h"
#include "../../str.h"
#include "../../dprint.h"
#include "../../error.h"
#include "../../mem/mem.h"
#include "../auth/api.h"
#include "../auth_db/rfc2617.h"

/* ---- Constants -------------------- */
#define RESULT_YES 1
#define RESULT_DROP_PACKET 0
#define RESULT_NO -1
#define MESSAGE_500 "Server Internal Error"


/* ---- Warning ---------------------- */
#warning "====================================="
#warning ""
#warning "LDAP module is still experimental and may crash SER."
#warning "or create unexpected results. You use the module at your"
#warning "own risk. Please submit bugs at http://bugs.sip-router.org/"
#warning "or directly to the maintainer."
#warning ""
#warning "====================================="



/* ---- Some module globals ----------- */
static db_con_t *db_con;
char *db_url = DEFAULT_DB_URL;
static char rpid_buffer[MAX_RPID_LEN];
static str rpid = {rpid_buffer, 0};
LDAP* ld;
MODULE_VERSION

pre_auth_f pre_auth_func = 0;
post_auth_f post_auth_func = 0;

/* Pointer to reply function in stateless module */
int (*sl_reply)(struct sip_msg* _msg, char* _str1, char* _str2);



/* ---- Prototypes -------------------- */

/* -- Defined in ldap.h -- */
static char* get_filter( str, char*, char* );
static char* get_user_filter( char*, char*, char* );
static int search_filter( char* );
static int hf_fixup(void**, int ); /* Header field fixup */
static int str_fixup(void**, int );
static int is_user_in_group( str, str );
static int set_uri (const char*, struct sip_msg* );
static int rewrite_lang( str, str, struct sip_msg* );
static int rewrite_alias( str, str, struct sip_msg* );
static int rewrite_prefix( str, str, int, struct sip_msg* );
static char* get_default_prefix( str, int);

/* -- Defined in ldap.c -- */
static int ldap_does_uri_exist_f( struct sip_msg* );
static int mod_init( void );
static int child_init( int );
static void mod_destroy( void );
static int ldap_is_user_in_f( struct sip_msg*, char*, char* );
static int ldap_www_authorize_f( struct sip_msg*, char* );
static int ldap_alias_lookup_f( struct sip_msg* );
static int ldap_lang_lookup_f( struct sip_msg* );
static int ldap_prefixes_lookup_f( struct sip_msg*, char* );
static int ldap_www_authorize_f( struct sip_msg*, char* );
static int ldap_proxy_authorize_f( struct sip_msg*, char* );



/* ---- Module parameters ---------------- */
static char *LDAP_Server = "127.0.0.1";
static int LDAP_Port = 389;
static char* LDAP_Base = "";
static char* LDAP_Admin = "";
static char* LDAP_Passwd = "";
static char* LDAP_SIP_objectClass = "";
static char* LDAP_SIP_userAttrib = "";
static char* LDAP_SIP_grpAttrib = "";
static char* LDAP_langAttrib = "preferredLanguage";
static char* LDAP_SIP_aliasAttrib = "";
static char* LDAP_SIP_CcPrefixAttrib = "";
static char* LDAP_SIP_LacPrefixAttrib = "";
static char* LDAP_SIP_passwdAttrib = "";
static int calc_ha1 = 1;          /* if set to 1, ha1 is calculated by the server */

/* ---- Module exports ------------- */

/* -- Command exports -- */
/* Allowed route flags are: */
/* REQUEST_ROUTE 1 Function can be used in request route blocks */
/* FAILURE_ROUTE 2 Function can be used in reply route blocks */
/* ONREPLY_ROUTE 4 Function can be used in on_reply */

static cmd_export_t cmds[] = {
	{"ldap_does_uri_exist", (cmd_function)ldap_does_uri_exist_f, 0, 0, REQUEST_ROUTE},
	{"ldap_is_user_in", ldap_is_user_in_f, 2, hf_fixup, REQUEST_ROUTE},
	{"ldap_www_authorize", (cmd_function)ldap_www_authorize_f, 1, 0, REQUEST_ROUTE},
	{"ldap_alias_lookup", (cmd_function)ldap_alias_lookup_f, 0, 0, 3},	/* in REQUEST and FAILURE */
	{"ldap_lang_lookup", (cmd_function)ldap_lang_lookup_f, 0, 0, REQUEST_ROUTE},
	{"ldap_prefixes_lookup", (cmd_function)ldap_prefixes_lookup_f, 1, 0, REQUEST_ROUTE},
	{"ldap_www_authorize",   (cmd_function)ldap_www_authorize_f,   1, str_fixup, REQUEST_ROUTE},
	{"ldap_proxy_authorize", (cmd_function)ldap_proxy_authorize_f, 1, str_fixup, REQUEST_ROUTE},
	{0, 0, 0, 0, 0}
};

/* -- Parameter exports -- */
static param_export_t params[] = {
	{"ldap_server", STR_PARAM, &LDAP_Server},
	{"ldap_port", INT_PARAM, &LDAP_Port},
	{"ldap_base", STR_PARAM, &LDAP_Base},
	{"ldap_admin", STR_PARAM, &LDAP_Admin},
	{"ldap_passwd", STR_PARAM, &LDAP_Passwd},
	{"ldap_sip_objectclass", STR_PARAM, &LDAP_SIP_objectClass},
	{"ldap_sip_user_attrib", STR_PARAM, &LDAP_SIP_userAttrib},
	{"ldap_sip_grp_attrib", STR_PARAM, &LDAP_SIP_grpAttrib},
	{"ldap_lang_attrib", STR_PARAM, &LDAP_langAttrib},
	{"ldap_sip_alias_attrib", STR_PARAM, &LDAP_SIP_aliasAttrib},
	{"ldap_sip_cc_prefix_attrib", STR_PARAM, &LDAP_SIP_CcPrefixAttrib},
	{"ldap_sip_lac_prefix_attrib", STR_PARAM, &LDAP_SIP_LacPrefixAttrib},
	{"ldap_sip_passwd_attrib", STR_PARAM, &LDAP_SIP_passwdAttrib},
	{"calc_ha1", INT_PARAM, &calc_ha1},
	{0, 0, 0}
};

/* -- Struct module exports -- */
struct module_exports exports = {
      "ldap",
      cmds,
      params,
      mod_init,			/* module initialization function */
      0,			/* response function */
      mod_destroy,		/* destroy function */
      0,			/* oncancel function */
      child_init		/* per-child init function */
};





/* ----------------------------------	*/
/* Helpful inline functions		*/
/* ----------------------------------	*/

/* ------------------------	*/
/*  Get actual Request-URI	*/
/* ------------------------	*/
static inline int get_request_uri(struct sip_msg* _m, str* _u) {

	/* Use new_uri if present */
	if (_m->new_uri.s) {     
		_u->s = _m->new_uri.s;
		_u->len = _m->new_uri.len;
	} else {		 
		_u->s = _m->first_line.u.request.uri.s;
		_u->len = _m->first_line.u.request.uri.len;
	}

	return 0;		
}


/* ------------------------	*/
/* Get To header field URI	*/
/* ------------------------	*/
static inline int get_to_uri(struct sip_msg* _m, str* _u) {

	/* Double check that the header field is there and is parsed */		 
	if (!_m->to && ((parse_headers(_m, HDR_TO, 0) == -1) || !_m->to)) {
		LOG(L_ERR, "ERROR: ldap - Can't get To header field in get_to_uri()\n");
		return -1;       
	}

	_u->s = ((struct to_body*)_m->to->parsed)->uri.s;
	_u->len = ((struct to_body*)_m->to->parsed)->uri.len;

	return 0;		
}

/* --------------------------	*/
/* Get From header field URI	*/
/* -------------------------	*/
static inline int get_from_uri(struct sip_msg* _m, str* _u)
{
	/* Double check that the header field is there and is parsed */		 
	if (parse_from_header(_m) < 0) {
		LOG(L_ERR, "ERROR: ldap - Error while parsing From body in get_from_uri()\n");
		return -1;
	}

	_u->s = ((struct to_body*)_m->from->parsed)->uri.s;
	_u->len = ((struct to_body*)_m->from->parsed)->uri.len;

	return 0;		
}


/* ----------------------------------------------------------------------	*/
/* Other useful functions							*/
/* ----------------------------------------------------------------------	*/

/* ---------------------------------------------------
 * Convert HF description string to hdr_field pointer
 *
 * Supported strings:
 * "Request-URI", "To", "From", "Credentials"
 * --------------------------------------------------
*/

static int hf_fixup(void** param, int param_no)
{
	void* ptr;
	str* s;

	if (param_no == 1) {
		ptr = *param;
		
		if (!strcasecmp((char*)*param, "Request-URI")) {
			*param = (void*)1;
		} else if (!strcasecmp((char*)*param, "To")) {
			*param = (void*)2;
		} else if (!strcasecmp((char*)*param, "From")) {
			*param = (void*)3;
		} else if (!strcasecmp((char*)*param, "Credentials")) {
			*param = (void*)4;
		} else {
			LOG(L_ERR, "ERROR: ldap - Unsupported Header Field identifier in hf_fixup()\n");
			return E_UNSPEC;
		}

		pkg_free(ptr);
	} else if (param_no == 2) {
		s = (str*)pkg_malloc(sizeof(str));   
		if (!s) {
			LOG(L_ERR, "ERROR: ldap - No memory left in hf_fixup()\n");
			return E_UNSPEC;
		}
		   
		s->s = (char*)*param;
		s->len = strlen(s->s);
		*param = (void*)s;
	}

	return 0;  
}


/* -------------------------------------------
 * Convert char* parameter to str* parameter
 * 
 * -------------------------------------------
*/
static int str_fixup(void** param, int param_no) {
	str* s;

	if (param_no == 1) {
		s = (str*)pkg_malloc(sizeof(str));
		if (!s) {
			LOG(L_ERR, "ldap - No memory left in str_fixup()\n");
			return E_UNSPEC;
		}  

		s->s = (char*)*param;
		s->len = strlen(s->s);
		*param = (void*)s;
	}

	return 0;
}



/* -----------------------------------------------------------------------	*/
/* Construct LDAP filter, with objectClass and attributeType, if provided	*/
/* -----------------------------------------------------------------------	*/

static char* get_filter( str to, char* pObjType, char* pAttrib ) {
	char tmp[255];
	int start, stop;
	int i;

	start = 0;
	i = 0;

	if (strncmp (to.s, "sip:", 4) == 0)
		{
			start = 0;
		}
	else
		{

			while ((to.s[i] != '<') && (i < to.len))
	{
		start++;
		i++;
	}
			start++;
		}

	/* NOTE: Added ':' for problems with port number	*/
	/* in Request-URI of some Korean devices	*/
	while( ((to.s[i] != '>') && (to.s[i] != ';') && !( (to.s[i] == ':') && ( i>3 )) ) && (i < to.len))
		{
			i++;
		}
	stop = i;

	if ((to.s[start] == 's') && (to.s[start + 1] == 'i')
			&& (to.s[start + 2] == 'p') && (to.s[start + 3] = ':'))
		{
			start += 4;
		}

	if (start > stop)
		{
			LOG(L_ERR, "ERROR: ldap - invalid To URI in get_filter()\n");
			/* return ""; */
			return NULL;
		}

	/* Check if objectClass and attributeType were provided */

	if( (pObjType != NULL) && (pObjType != "") && (strlen( pObjType) > 0)) {
		if( (pAttrib != NULL) && (pAttrib != "") && (strlen( pAttrib) > 0)) {
			strcpy( tmp, "(&(objectClass=");
			strcat( tmp, pObjType);
			strcat( tmp, ")(");
			strcat( tmp, pAttrib);
			strcat( tmp, "=");
			strncat( tmp, &to.s[start], stop - start);
			strcat( tmp, "))");
		} else {
			strcpy( tmp, "(objectClass=");
			strcat( tmp, pObjType);
			strcat( tmp, ")");
		}
	} else {
		if( (pAttrib != NULL) && (pAttrib != "") && (strlen( pAttrib) > 0)) {
			strcpy( tmp, "(");
			strcat( tmp, pAttrib);
			strcat( tmp, "=");
			strncat( tmp, &to.s[start], stop - start);
			strcat( tmp, ")");
		} else {
			strcpy( tmp, "(objectClass=*)");
		}
	}

	return strdup (tmp);
}


static char* get_user_filter( char* user, char* pObjType, char* pAttrib ) {

	char tmp[255];
	
	/* Check if objectClass and attributeType were provided */

	if( (pObjType != NULL) && (pObjType != "") && (strlen( pObjType) > 0)) {
		if( (pAttrib != NULL) && (pAttrib != "") && (strlen( pAttrib) > 0)) {
			strcpy( tmp, "(&(objectClass=");
			strcat( tmp, pObjType);
			strcat( tmp, ")(");
			strcat( tmp, pAttrib);
			strcat( tmp, "=");
			strcat( tmp, user );
			strcat( tmp, "))");
		} else {
			strcpy( tmp, "(objectClass=");
			strcat( tmp, pObjType);
			strcat( tmp, ")");
		}
	} else {
		if( (pAttrib != NULL) && (pAttrib != "") && (strlen( pAttrib) > 0)) {
			strcpy( tmp, "(");
			strcat( tmp, pAttrib);
			strcat( tmp, "=");
			strcat( tmp, user );
			strcat( tmp, ")");
		} else {
			strcpy( tmp, "(objectClass=*)");
		}
	}

	return strdup (tmp);
}




/* ----------------------------------------------------------------- 	*/
/* Search for user in ldap and if found, return 1. Otherwise return -1	*/
/* -----------------------------------------------------------------	*/

static int search_filter( char *filter ) {

	LDAPMessage *msg;
	int res;
	char *attrs[2];
	int retval = -1;

	/* if( filter == NULL ) { */
	if( !strcmp( filter, "" ) ) {
		LOG(L_ERR, "ERROR: ldap - Filter expresion empty in search_filter()\n" );
		return -1;
	}

	attrs[0] = NULL;
	res = ldap_search_s( ld, LDAP_Base, LDAP_SCOPE_SUBTREE, filter, attrs, 0, &msg);
	if (res != LDAP_SUCCESS) {
		LOG(L_ERR, "ERROR: ldap - Error '%i %s' in search for filter '%s', in base '%s' in search_filter()\n", res, ldap_err2string(res), filter, LDAP_Base );
		ldap_msgfree (msg);
		return retval;
	}

	if( ldap_count_entries( ld, msg )< 1) {
		; /* Nothing, retval = -1; */
	} else {
		retval = 1;
	}
	
	ldap_msgfree (msg);
	/* free (filter); */
	return retval;
}

/* -------------------------------------------	*/
/* Search for user in LDAP, check if his group	*/
/* attribute contains group value. Return 1 on	*/
/* success or -1 on failure			*/
/* -------------------------------------------	*/
static int is_user_in_group( str user, str group) {

	LDAPMessage *msg;
	LDAPMessage* entry;
	int res, valcount, tmp;
	char *attrs[2];
	char* filter = NULL;
	char* attr;
	char** vals;
	BerElement *ber;
	int retval = -1;
	
	attrs[0] = LDAP_SIP_grpAttrib;
	attrs[1] = NULL;

	filter = get_filter( user, LDAP_SIP_objectClass, LDAP_SIP_userAttrib );

	if( filter == NULL ) {
		LOG(L_ERR, "ERROR: ldap - Filter expresion empty in is_user_in_group()\n" );
		return retval;
	}

	res = ldap_search_s( ld, LDAP_Base, LDAP_SCOPE_SUBTREE, filter, attrs, 0, &msg);
	if (res != LDAP_SUCCESS) {
		LOG(L_ERR, "ERROR: ldap - Error '%i %s' in search for filter '%s', in base '%s' in is_user_in_group()\n", res, ldap_err2string(res), filter, LDAP_Base );
		free(filter);
		ldap_msgfree (msg);
		return retval;
	}

	if( ldap_count_entries( ld, msg ) < 1 ) {
		ldap_msgfree (msg);
		free (filter);
		return retval;
	}

	entry = ldap_first_entry (ld, msg);
	attr = ldap_first_attribute (ld, entry, &ber);

	if( attr != NULL ) {
		vals = ldap_get_values (ld, entry, attr);
		valcount = ldap_count_values(vals);
		for( tmp = 0; tmp < valcount; tmp++ ) {
			if( strcasecmp( vals[tmp], group.s ) == 0 ) {
				/* LOG( L_INFO, "Hit in is_user_in_group: %s: %s count: %i, group: %s\n", attr, vals[tmp],valcount, group.s); */
				retval = 1;
				break;
			}
		}
		ldap_value_free (vals);
	}
	
	ber_free( ber, 0 );
	ldap_msgfree( msg );
	ldap_memfree( attr );
	free( filter );
	return retval;
}

/* -----------------------------------	*/
/* Alter msg so uri is called.		*/
/* Return 1 on success, -1 on failure	*/
/* -----------------------------------	*/
static int set_uri (const char *uri, struct sip_msg *msg)
{
  struct action act;
  int ret;

  memset( &act, 0, sizeof (act));
  act.type = SET_URI_T;
  act.p1_type = STRING_ST;
  act.p1.string = strdup( uri );
  ret = do_action( &act, msg);

  if (ret < 0)
    {
      LOG (L_ERR, "ERROR: ldap - unable to set uri (error=%i)\n", ret);
      ret = -1;
    }
  else
    {
      ret = 1;
    }
  return ret;
}

/* -------------------------------------------		*/
/* Search for user in LDAP, check his default		*/
/* language attribute. If found, rewrite Request-URI	*/
/* and return 1. Ona failure, return -1, do not 	*/
/* rewrite Request-URI.					*/
/* -------------------------------------------		*/
static int rewrite_lang( str user, str request_uri, struct sip_msg* pSipMsg ) {

	LDAPMessage *msg;
	LDAPMessage* entry;
	int res, intSize;
	char *attrs[2];
	char* filter = NULL;
	char* attr;
	char* strNewUri;
	char** vals;
	BerElement *ber;
	int retval = -1;
	
	attrs[0] = LDAP_langAttrib;
	attrs[1] = NULL;

	filter = get_filter( user, LDAP_SIP_objectClass, LDAP_SIP_userAttrib );

	if( filter == NULL ) {
		LOG(L_ERR, "ERROR: ldap - Filter expresion empty in rewrite_lang()\n" );
		return retval;
	}

	res = ldap_search_s( ld, LDAP_Base, LDAP_SCOPE_SUBTREE, filter, attrs, 0, &msg);
	if (res != LDAP_SUCCESS) {
		LOG(L_ERR, "ERROR: ldap - Error '%i %s' in search for filter '%s', in base '%s' in rewrite_lang()\n", res, ldap_err2string(res), filter, LDAP_Base );
		free( filter );
		ldap_msgfree (msg);
		return retval;
	}

	if( ldap_count_entries( ld, msg ) < 1 ) {
		ldap_msgfree (msg);
		free (filter);
		return retval;
	}

	entry = ldap_first_entry (ld, msg);
	attr = ldap_first_attribute (ld, entry, &ber);

	if( attr != NULL ) {
		vals = ldap_get_values (ld, entry, attr);
		if( vals[0] ) {
			intSize = strlen(vals[0]) + request_uri.len;
			strNewUri = (char*) calloc( intSize, sizeof(char));
			/* strNewUri = (char*) pkg_malloc( intSize ); */
			strcat( strNewUri, "sip:" );
			strcat( strNewUri, vals[0] );
			strncat( strNewUri, (request_uri.s)+4, request_uri.len-4 );

			if( set_uri( strNewUri, pSipMsg ) ) {
				retval = 1;
			}
			free(strNewUri);
			/* pkg_free(strNewUri); */
		}
		ldap_value_free (vals);
	}
	
	ber_free( ber, 0 );
	ldap_msgfree( msg );
	ldap_memfree( attr );
	free( filter );
	return retval;
}

/* -------------------------------------------		*/
/* Search for alias in LDAP. If found, rewrite		*/
/* Request URI and return 1. On failure, return -1 	*/
/* do not rewrite Request URI.				*/
/* -------------------------------------------		*/
static int rewrite_alias( str request_uri, str from_uri, struct sip_msg* pSipMsg ) {

	LDAPMessage *msg;
	LDAPMessage* entry;
	int res, retval, intSize, intMode;
	char *attrs[2];
	char* filter = NULL;
	char* attr;
	char* strNewUri;
	char** vals;
	char* prefix = NULL;
	BerElement *ber;
	struct sip_uri puri;
	char temp[255];

	attrs[0] = LDAP_SIP_userAttrib;
	attrs[1] = NULL;
	retval = -1;

	if (parse_uri(request_uri.s, request_uri.len, &puri) < 0 || puri.user.s == NULL || puri.user.len <= 0 ) {
		LOG(L_ERR, "ERROR: ldap - Error while parsing URI in rewrite_alias()\n");
		return retval;
	}

	/* Check what kind of number was dialed */
	if( strncasecmp( puri.user.s, "0", 1 ) == 0 ) {
		if( strncasecmp( puri.user.s, "00", 2 ) == 0 ) {
			/* Full E.164 number, i.e. 00+CC+LAC+xxxxxxx */
			intMode = 2;
		} else {
			/* Number with local area code prefix, i.e. 0+LAC+xxxxxx */
			intMode = 3;
		}
	} else {
		/* Number without prefixes, i.e. xxxxxx */
		intMode = 4;
	}

	switch( intMode ) {

		case 2:
			strcpy( temp, "00");
			strncat( temp,  puri.user.s+2, puri.user.len-2);
			filter = get_user_filter( temp, LDAP_SIP_objectClass, LDAP_SIP_aliasAttrib );
			break;
		case 3:
			if( (prefix = get_default_prefix( from_uri, 3)) == NULL ) {
				return retval;
			} else {
				strcpy( temp,"00");
				strcat( temp, prefix);
				strncat( temp, puri.user.s+1, puri.user.len-1);
				filter = get_user_filter( temp, LDAP_SIP_objectClass, LDAP_SIP_aliasAttrib );
			}
			break;
		case 4:
			if( (prefix = get_default_prefix( from_uri, 4)) == NULL ) {
				return retval;
			} else {
				strcpy( temp, "00" );
				strcat( temp, prefix);
				strncat( temp, puri.user.s, puri.user.len);
				filter = get_user_filter( temp, LDAP_SIP_objectClass, LDAP_SIP_aliasAttrib );
			}
			break;
	
		default:
			return retval;
			break;	
	}

	if( filter == NULL ) {
		LOG(L_ERR, "ERROR: ldap - Filter expresion empty in rewrite_lang()\n" );
		free(prefix);
		return retval;
	}

	res = ldap_search_s( ld, LDAP_Base, LDAP_SCOPE_SUBTREE, filter, attrs, 0, &msg);
	if (res != LDAP_SUCCESS) {
		LOG(L_ERR, "ERROR: ldap - Error '%i %s' in search for filter '%s', in base '%s' in rewrite_alias()\n", res, ldap_err2string(res), filter, LDAP_Base );
		ldap_msgfree (msg);
		free(prefix);
		free(filter);
		return retval;
	}

	if( ldap_count_entries( ld, msg ) < 1 ) {
		ldap_msgfree (msg);
		free (filter);
		free(prefix);
		return retval;
	}

	entry = ldap_first_entry (ld, msg);
	attr = ldap_first_attribute (ld, entry, &ber);

	if( attr != NULL ) {
		vals = ldap_get_values (ld, entry, attr);
		if( vals[0] ) {
			intSize = 4 + strlen(vals[0]);
			strNewUri = (char*) calloc( intSize, sizeof(char));
			/* strNewUri = (char*) pkg_malloc( intSize ); */
			strcat( strNewUri, "sip:" );
			strcat( strNewUri, vals[0] );
			if( set_uri( strNewUri, pSipMsg ) ) {
				retval = 1;
			}
			free(strNewUri);
			/* pkg_free(strNewUri); */
		}
		ldap_value_free (vals);
	}
	
	ber_free( ber, 0 );
	ldap_msgfree( msg );
	ldap_memfree( attr );
	free( filter );
	free(prefix);
	return retval;
}

/* -------------------------------------------	*/
/* Search for user in LDAP. If found, check his */
/* default prefixes and add to Request URI	*/
/* If no default prefixes or one of them is 	*/
/* missing accordingly, do not rewrite and	*/
/* return -1					*/
/* -------------------------------------------	*/
static int rewrite_prefix( str from_uri, str request_uri, int intMode, struct sip_msg* pSipMsg ) {

	int intSize;
	char* strNewUri;
	char* prefix = NULL;
	int retval = -1;

	/* Check what kind of prefix is needed (from calling function) */
	switch( intMode ) {

		case 3:
			/* Country Code prefix */
			if( (prefix = get_default_prefix( from_uri, 3)) == NULL ) {
				return retval;
			}

			/* Allocate for 'sip:00', prefix, uri (uri comes with redundant 0)*/
			intSize = 1 + strlen(prefix) + request_uri.len;
			strNewUri = (char*) calloc( intSize, sizeof(char));
			/* strNewUri = (char*) pkg_malloc( intSize ); */
			strcpy( strNewUri, "sip:00" );
			strcat( strNewUri, prefix );
			strncat( strNewUri, request_uri.s+5, request_uri.len-5);
			break;
		case 4:
			/* Country Code and Local Area Code */
			if( (prefix = get_default_prefix( from_uri, 4)) == NULL ) {
				return retval;
			}

			/* Allocate for 'sip:00', prefix, uri */
			intSize = 2 + strlen(prefix) + request_uri.len;
			strNewUri = (char*) calloc( intSize, sizeof(char));
			/* strNewUri = (char*) pkg_malloc( intSize ); */
			strcpy( strNewUri, "sip:00" );
			strcat( strNewUri, prefix );
			strncat( strNewUri, request_uri.s+4, request_uri.len-4);
			break;
		default:
			return retval;
			break;
	}

	if( set_uri( strNewUri, pSipMsg ) ) {
		retval = 1;
	}

	free(strNewUri);
	free(prefix);
	return retval;
}


static char* get_default_prefix( str user, int intMode ) {

	char* strRet = NULL;
	LDAPMessage *msg;
	LDAPMessage* entry;
	BerElement *ber;
	char* attrs[3];
	char* filter = NULL;
	char temp[255];
	char* attr;
	int res, count;
	char** vals;
		
	switch( intMode ) {

		case 3:
		attrs[0] = LDAP_SIP_CcPrefixAttrib;
		attrs[1] = NULL;
		break;

		case 4:
		attrs[0] = LDAP_SIP_CcPrefixAttrib;
		attrs[1] = LDAP_SIP_LacPrefixAttrib;
		attrs[2] = NULL;
		break;
		
		default:
		return NULL;
		break;
	}

	filter = get_filter( user, LDAP_SIP_objectClass, LDAP_SIP_userAttrib );

	if( filter == NULL ) {
		LOG(L_ERR, "ERROR: ldap - Filter expresion empty in get_default_prefix()\n" );
		return NULL;
	}

	res = ldap_search_s( ld, LDAP_Base, LDAP_SCOPE_SUBTREE, filter, attrs, 0, &msg);
	if (res != LDAP_SUCCESS) {
		LOG(L_ERR, "ERROR: ldap - Error '%i %s' in search for filter '%s', in base '%s' in get_default_prefix()\n", res, ldap_err2string(res), filter, LDAP_Base );
		free( filter );
		ldap_msgfree (msg);
		return NULL;
	}

	if( ldap_count_entries( ld, msg ) < 1 ) {
		ldap_msgfree (msg);
		free (filter);
		return NULL;
	}

	entry = ldap_first_entry (ld, msg);
	attr = ldap_first_attribute (ld, entry, &ber);

	switch( intMode ) {

		case 3:
			/* Only Country Code */
			if( attr != NULL) {
				vals = ldap_get_values (ld, entry, attr);
				if( vals[0] ) {
					strRet = strdup( vals[0] );
				}
				ldap_value_free (vals);
			}
			break;
		
		case 4:
			/* Country Code and Local Area Code */
			count = 0;
			strcpy( temp, "" );
			while( attr != NULL ) {
				vals = ldap_get_values (ld, entry, attr);
				if( vals[0] ) {
					strcat( temp, vals[0] );
					count++;
				}
				ldap_value_free (vals);
				attr = ldap_next_attribute (ld, entry, ber);
			}

			if( count == 2 ) {
				strRet = strdup( temp );
			}
			break;
	}

	ber_free( ber, 0 );
	ldap_msgfree( msg );
	ldap_memfree( attr );
	free( filter );
	return strRet;
}

/* --------------------------------------------------
 * Retrieve password from LDAP dbase, calculate hash
 * --------------------------------------------------
*/
static inline int get_ha1(struct username* _username, str* _domain, char* _ha1 ) {

	str result;
	LDAPMessage *msg;
	LDAPMessage* entry;
	int res;
	char *attrs[2];
	char user_temp[255];
	char* filter = NULL;
	char* attr;
	char** vals = NULL;
	BerElement *ber;
	
	attrs[0] = LDAP_SIP_passwdAttrib;
	attrs[1] = NULL;

	/* Prepare username (number@domain) */
	strcpy( user_temp, "");
	strncat( user_temp, _username->user.s, _username->user.len) ;
	strcat( user_temp, "@");
	strncat( user_temp, _domain->s, _domain->len );

	filter = get_user_filter( user_temp, LDAP_SIP_objectClass, LDAP_SIP_userAttrib );

	/* Not used
		if( filter == "" ) {
			LOG(L_ERR, "ERROR: ldap - Filter expresion empty in get_ha1()\n" );
			return -1;
		}
	*/
	
	res = ldap_search_s( ld, LDAP_Base, LDAP_SCOPE_SUBTREE, filter, attrs, 0, &msg);
	if (res != LDAP_SUCCESS) {
		LOG(L_ERR, "ERROR: ldap - Error '%i %s' in search for filter '%s', in base '%s' in get_ha1()\n", res, ldap_err2string(res), filter, LDAP_Base );
		ldap_msgfree (msg);
		free(filter);
		return -1;
	}

	if( ldap_count_entries( ld, msg ) < 1 ) {
		DBG("ldap - no LDAP result for user %s in get_ha1()", user_temp);
		ldap_msgfree (msg);
		free (filter);
		return 1;
	}

	entry = ldap_first_entry (ld, msg);
	attr = ldap_first_attribute (ld, entry, &ber);
	
	if( attr != NULL ) {
		vals = ldap_get_values (ld, entry, attr);
		if( !vals[0] ) {
			/* No value for attribute in result */
			DBG("ldap - attribute has no value in LDAP result for user %s in get_ha1()", user_temp);
			ber_free( ber, 0 );
			ldap_msgfree( msg );
			ldap_memfree( attr );
			ldap_value_free (vals);
			free( filter );
			return 1;
		}
	} else {
		/* No attribute in result */
		DBG("ldap - no attribute in LDAP result for user %s in get_ha1()", user_temp);
		ber_free( ber, 0 );
		ldap_msgfree( msg );
		ldap_memfree( attr );
		ldap_value_free (vals);
		free( filter );
		return 1;
	}

	result.s = (char*)vals[0];
	result.len = strlen(vals[0]);

	if( calc_ha1 ) {
		     /* Only plaintext passwords are stored in database,
		      * we have to calculate HA1 */
		calc_HA1(HA_MD5, &_username->whole, _domain, &result, 0, 0, _ha1);
		DBG("HA1 string calculated: %s\n", _ha1);
	} else {
		memcpy(_ha1, result.s, result.len);
		_ha1[result.len] = '\0';
	}

	ber_free( ber, 0 );
	ldap_msgfree( msg );
	ldap_memfree( attr );
	ldap_value_free (vals);
	free( filter );
	return 0;
}


/* -----------------------------------------------------------------
 * Calculate the response and compare with the given response string
 * Authorization is successfull if this two strings are same
 * -----------------------------------------------------------------
 */
static inline int check_response(dig_cred_t* _cred, str* _method, char* _ha1) {

	HASHHEX resp, hent;
  
	     /*
	      * First, we have to verify that the response received has
	      * the same length as responses created by us
	      */
	if (_cred->response.len != 32) {
		DBG("check_response(): Receive response len != 32\n");
		return 1;
	}

	     /*
	      * Now, calculate our response from parameters received
	      * from the user agent
	      */
	calc_response(_ha1, &(_cred->nonce),
		      &(_cred->nc), &(_cred->cnonce),
		      &(_cred->qop.qop_str), _cred->qop.qop_parsed == QOP_AUTHINT,
		      _method, &(_cred->uri), hent, resp);

	DBG("check_response(): Our result = \'%s\'\n", resp);
	
	     /*
	      * And simply compare the strings, the user is
	      * authorized if they match
	      */
	if (!memcmp(resp, _cred->response.s, 32)) {
		DBG("check_response(): Authorization is OK\n");
		return 0;    
	} else {
		DBG("check_response(): Authorization failed\n");
		return 2;    
	}
}


/* -----------------------------
 * Authorize digest credentials
 * -----------------------------
 */
static inline int authorize(struct sip_msg* _m, str* _realm, int _hftype) {
	char ha1[256];
	int res;
	struct hdr_field* h;
	auth_body_t* cred;
	auth_result_t ret;
	str domain;

	domain = *_realm;

	/*
	 * Purpose of this function is to find credentials with given realm,
	 * do sanity check, validate credential correctness and determine if
	 * we should really authenticate (there must be no authentication for
	 * ACK and CANCEL
	 */

	ret = pre_auth_func(_m, &domain, _hftype, &h);

	switch(ret) {
		case ERROR:	    return 0;
		case NOT_AUTHORIZED:   return -1;
		case DO_AUTHORIZATION: break;
		case AUTHORIZED:       return 1;
	}

	cred = (auth_body_t*)h->parsed;

	/* Clear the rpid buffer */
	rpid.len = 0;

	res = get_ha1(&cred->digest.username, &domain, ha1 );
	if (res < 0) {
		/* Error while accessing the database */
		if (sl_reply(_m, (char*)500, MESSAGE_500) == -1) {
			LOG(L_ERR, "ERROR: ldap - Error while sending 500 reply in authorize()\n");
		}
		return 0;
	} else if (res > 0) {
		/* Username not found in the database */
		return -1;
	}

	/* Recalculate response, it must be same to authorize sucessfully */
	if (!check_response(&(cred->digest), &_m->first_line.u.request.method, ha1)) {
		/*
		 * Purpose of this function is to do post authentication steps like
		 * marking authorized credentials and so on.
		 */
		ret = post_auth_func(_m, h, &rpid);
		switch(ret) {
			case ERROR:		return 0;
			case NOT_AUTHORIZED:	return -1;
			case AUTHORIZED:	return 1;
			default:		return -1;
		}
	}
	
	return -1;
}
