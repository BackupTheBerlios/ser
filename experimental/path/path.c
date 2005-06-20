/* 
 * 
 * PATH MODULE
 *
 * This module read the Path from the sip message (store_path
 * function) and perform Path-based routing (path_based_route
 * function)
 * 
 * Copyright (C) 2005, Agora Systems S. A.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 *
 * An online copy of the licence can be found at
 * http://www.gnu.org/copyleft/gpl.html
 * 
 * Authors:
 * 
 * - Carlos Garcia Santos (carlos.garcia@agora-2000.com)
 * - Fermín Galán Márquez (fermin.galan@agora-2000.com)
 *
 * NOTES:
 *
 * - use pkg_malloc and pkg_free instead of standard C
 *   functions to allocate memory
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../sr_module.h"
#include "../../error.h"
#include "../../dprint.h"
#include "../../mem/mem.h"
#include "../../parser/msg_parser.h"
#include "../../parser/parse_uri.h"
#include "../../parser/hf.h"
#include "../../parser/parse_rr.h"
#include "../../proxy.h"
#include "../../forward.h"  // Yo need to comment #ifdef clause in route.h to get compile! FIXME
#include "../../data_lump.h"
#include "../../ut.h"
#include "../rr/loose.h"
#include "config.h"
#include "path.h"

MODULE_VERSION

// Pointer to functions, set up by mod_init
// - t_relay_to_udp and t_relay_to_tcp in tm module
// - append_hf in textopts module
cmd_function t_relay_to_udp_f, t_relay_to_tcp_f, append_hf_f;

// Stores the path_db file 
char* db_file = "/tmp/path_db";

/*
 * Exported functions
 */
static cmd_export_t cmds[] = {
 	// Scope note: REQUEST_ROUTE|ONREPLY_ROUTE|FAILURE_ROUTE
	{"path_based_route",path_based_route, 0, 0, REQUEST_ROUTE},
	{"store_path",store_path, 0, 0 , REQUEST_ROUTE},
	//{"test_header",test_header, 0, 0 , REQUEST_ROUTE|ONREPLY_ROUTE|FAILURE_ROUTE},
	//{"test_routing",test_routing, 0, 0 , REQUEST_ROUTE|ONREPLY_ROUTE|FAILURE_ROUTE},
	{0, 0, 0, 0, 0}
};


/*
 * Exported parameters
 */
static param_export_t params[] = {
	//{"db_file",STR_PARAM,&path},
	{0, 0, 0}    
};

/*
 * Module exports structure
 */
struct module_exports exports = {
   "path", 
   cmds,       /* Exported functions */
   params,     /* Exported parameters */
   mod_init,   /* module initialization function */
   0,          /* response function */
   destroy,    /* destroy function */
   0,          /* oncancel function */
   0           /* Per-child init function */
};

/**
 * Function for debug strings
 */
static void log_string_chars(char* s) {

    int i;
    for (i = 0; ;i++) {
        LOG(L_INFO,"%2d: '%c' (%X)\n",i,s[i],s[i]);
        if (s[i] == '\0') {
            break;
        }
    }
    
}

/*
 * Copy-pasted from modules/rr/loose.c with little modification (in the
 * LOG messages)
 * 
 * Check if URI is myself
 */
#ifdef ENABLE_USER_CHECK
static inline int is_myself(str *_user, str* _host, unsigned short _port)
#else
static inline int is_myself(str* _host, unsigned short _port)
#endif
{
	int ret;
#ifdef ENABLE_USER_CHECK
	if(i_user.len && i_user.len==_user->len
			&& !strncmp(i_user.s, _user->s, _user->len))
	{
		DBG("PATH:is_myself: this URI isn't mine\n");
		return 0;
	}
#endif
	ret = check_self(_host, _port ? _port : SIP_PORT);
	if (ret < 0) return 0;
	else return ret;
}

/*
 * Copy-pasted from modules/rr/loose.c verbatim
 * 
 * Parse the message and find first occurence of
 * Route header field. The function returns -1 or -2 
 * on a parser error, 0 if there is a Route HF and
 * 1 if there is no Route HF.
 */
static inline int find_first_route(struct sip_msg* _m)
{
	if (parse_headers(_m, HDR_ROUTE, 0) == -1) {
		LOG(L_ERR, "find_first_route(): Error while parsing headers\n");
		return -1;
	} else {
		if (_m->route) {
			if (parse_rr(_m->route) < 0) {
				LOG(L_ERR, "find_first_route(): Error while parsing Route HF\n");
				return -2;
			}
			return 0;
		} else {
			DBG("find_first_route(): No Route headers found\n");
			return 1;
		}
	}
}

/*
 * Copy-pasted from modules/textops/textops.c verbatim
 * 
 * Convert char* parameter to str* parameter
 */
static int str_fixup(void** param, int param_no)
{
	str* s;

	s = (str*)pkg_malloc(sizeof(str));
	if (!s) {
		LOG(L_ERR, "str_fixup(): No memory left\n");
		return E_UNSPEC;
	}

	s->s = (char*)*param;
	s->len = strlen(s->s);
	*param = (void*)s;

	return 0;
}

/*
 * Copy-pasted from modules/tm/tm.c with little modification (in the LOG
 * mesages)
 * 
 * (char *hostname, char *port_nr) ==> (struct proxy_l *, -)  
 */
static int fixup_hostport2proxy(void** param, int param_no)
{
	unsigned int port;
	char *host;
	int err;
	struct proxy_l *proxy;
	str s;
	
	DBG("PATH module: fixup_hostport2proxy(%s, %d)\n", (char*)*param, param_no);
	if (param_no==1){
		DBG("PATH module: fixup_hostport2proxy: param 1.. do nothing, wait for #2\n");
		return 0;
	} else if (param_no==2) {

		host=(char *) (*(param-1)); 
		port=str2s(*param, strlen(*param), &err);
		if (err!=0) {
			LOG(L_ERR, "PATH module:fixup_hostport2proxy: bad port number <%s>\n",
				(char*)(*param));
			 return E_UNSPEC;
		}
		s.s = host;
		s.len = strlen(host);
		proxy=mk_proxy(&s, port, 0); /* FIXME: udp or tcp? */
		if (proxy==0) {
			LOG(L_ERR, "ERROR: fixup_hostport2proxy: bad host name in URI <%s>\n",
				host );
			return E_BAD_ADDRESS;
		}
		/* success -- fix the first parameter to proxy now ! */

		/* FIXME: janakj, mk_proxy doesn't make copy of host !! */
		/*pkg_free( *(param-1)); you're right --andrei*/
		*(param-1)=proxy;
		return 0;
	} else {
		LOG(L_ERR,"ERROR: fixup_hostport2proxy called with parameter #<>{1,2}\n");
		return E_BUG;
	}
}

/*
 * This function prints into path_db the ue and the stored path
 * if the ue exists the function change de path associated to this ue
 *
 * Parameters:
 *
 * - char* ue, ue to be stored
 * - char* path, path to be stored
 *
 * Return:
 *
 * - 0, all right
 * - negative value, error condition
 * 
 */
static int write_path(char* ue, char* path) 
{
        char* aux_file = "/tmp/aux";
        FILE *fp;
        FILE *aux;
        char str[100];
        char *line;
        char *path2;
        char parsed[50] = "";
        int ue_size;
        int parsed_size;

        aux = fopen(aux_file,"a+");
        if ((fp = fopen(db_file,"r")) == NULL)
        {
           LOG(L_ERR,"write_path(): error opening %s\n",aux_file);
           return -1;
        }else
        {
                int written = 0;
                do
                {
                        line = fgets(str,100,fp);
                        if (line != NULL) {
                                //use an aux file to write the couple ue-path
                                LOG(L_INFO,"write_path(): reading line -%s-\n",line);
                                ue_size = strlen(ue);
                                path2 = strchr(line,',');
                                strncpy(parsed,line,ue_size);
                                parsed_size = strlen(parsed);
                                if (strcmp(ue,parsed) != 0)
                                {
                                        fprintf(aux,"%s",line);
                                }else
                                {
                                        fprintf(aux,"%s,%s\n",ue,path);
                                        written = 1;  
                                }
                        }
                        else {
                            if (!written) {   // Only write if not previously written
                                // The end of file has been reached without
                                // finding the ue: adding new line
                                LOG(L_INFO,"write_path(): end of the file reached adding new UE\n");
                                fprintf(aux,"%s,%s\n",ue,path);
                            }
                        }
                }while(!feof(fp));
        }
        fclose(fp);
        fclose(aux);
        if (rename(aux_file,db_file) != 0) {
            LOG(L_ERR,"write_path(): Error renaming %s -> %s\n",aux_file,db_file);
            if (unlink(aux_file) != 0) {
               LOG(L_ERR, "write_path(): Error removing %s\n",aux_file);
            }
            return -2;
        }
           
        return 0;
}



/*
 * This function search the stored path in the data base file
 *
 * Parameter:
 *
 * - char ue*, the key to search
 * 
 * Returns:
 *
 * - char*, the stored path, or null if not found
 * 
 */

static char* search_path(char* key)
{

   	FILE *fp;
    char str[100];
    char *line;
    char *pcscf;
    char parsed[50] = "";
    int key_size;

    if ((fp = fopen(db_file,"r")) == NULL) {
       LOG(L_ERR,"search_path(): file %s can not be opened in read mode\n",db_file);
       return NULL;
    }
    else {
       do {
          line = fgets(str,100,fp);
          if (line !=NULL) {
            //To find ue
            key_size = strlen(key);
            pcscf = strchr(line,',');
            strncpy(parsed,line,key_size);
            if(strncmp(key,parsed,key_size) == 0) {
                // Allocating len - 1 (trailing \n) -1 (',') 
                // + 1 (trailing '\0')
                char* ret = pkg_malloc(strlen(pcscf)-1);
                strncpy(ret,pcscf+1,strlen(pcscf)-2);
                ret[strlen(pcscf)-1-1] = '\0';
                close(fp);
                return ret;
             }
           }
        } 
        while(line !=NULL);
        close(fp);
        return NULL;
    }
    
}

/*
 *
 * Removes the '<sip:' prefix and '>' in the given string.
 * For example, if the string is '<sip:ue@domain.com>' the
 * function will return 'eu@domain.com'.
 *
 * If the function doesn't starts with '<sip:' or ends
 * with '>', return null.
 *
 * Parameters:
 *
 * - char* string, the string to be parsed
 *
 * Returns:
 *
 * - char*, the parsed string
 * - null, if the parameter doesn't follow the '<sip:x>'
 *   pattern
 * 
 */
static char* remove_brackets(char* string) 
{
        char* ue;
        int size;

        // Check format
        if (!((strncmp(string,"<sip:",5) == 0) && ( strstr(string,">") == string+strlen(string)-1 )))
            return NULL;
        
        // Remove brackets
        // Allocating len - 1 (:) - 1 (>) + 1 (\0)
        size = strlen(strstr(string,":"))-1;
        ue = pkg_malloc(size);
        strncpy(ue,strstr(string,":")+1,size-1);

        // Add the trailing \0
        ue[size-1] = '\0';
        LOG(L_INFO,"remove_brackets(): returning -%s-\n",ue);
        
	    return ue;
}


/*
 * Removes the user prefix in the given SIPO URI string.
 * For example, if the string is 'ue@domain.com' the
 * function will return 'domain.com'
 *
 * If the function doesn't have a '@' that splits the
 * string, return null
 *
 * Parameters:
 *
 * - char* string, the string to be parsed
 *
 * Returns:
 *
 * - char*, the parsed string
 * - null, if the parameter doesn't follow the 'a@b' pattern
 * 
 */
static char* remove_user_prefix (char* string)
{
        char* ue; 
        int string_size;

        string_size = strlen(string);
        ue = strstr(string,"@");
        if (ue[0] != '@')
        {
                return NULL;
        }else
        {
                // Allocating len - 1 (@) + 1 (\0)
                char* ret = pkg_malloc(strlen(ue));
                strncpy(ret,ue+1,strlen(ue)-1);
                // Addint the trailing \0
                ret[strlen(ue)-1] = '\0';
                LOG(L_INFO,"remove_user_prefix(): returning -%s-\n",ue);
                return ret;
        }
} 

/*
 * Removes the loose-routing sufix (;lr) in the given SIP URI string.
 * For example, if the string is 'ue@domain.com;lr' the
 * function will return 'ue@domain.com'
 *
 * If the function doesn't end with ';lr', return null
 *
 * Parameters:
 *
 * - char* string, the string to be parsed
 *
 * Returns:
 *
 * - char*, the parsed string
 * - null, if the parameter doesn't follow the 'a;lr' pattern
 * 
 */
static char* remove_lr_sufix (char* string) {

    // Check format
    if (strstr(string,";lr") != string+strlen(string)-3)
       return NULL;
    
    // Allocating len - 3 (;lr) +1
    int size = strlen(string)-2;
    char* ret = pkg_malloc(size);
    strncpy(ret,string,size-1);

    // Add the trailing \0
    ret[size-1] = '\0';
    LOG(L_INFO,"remove_lr_sufix(): returning -%s-\n",ret);
        
	return ret;
    
}

/*
 * Removes the SIP URI prefix (sip:) in the given SIP URI string.
 * For example, if the string is 'sip:ue@domain.com' the
 * function will return 'ue@domain.com'
 *
 * If the function doesn't starts with 'sip:', return null
 *
 * Parameters:
 *
 * - char* string, the string to be parsed
 *
 * Returns:
 *
 * - char*, the parsed string
 * - null, if the parameter doesn't follow the 'sip:a' pattern
 * 
 */
static char* remove_sip_prefix (char* string) {

    // Check format
    if (strstr(string,"sip:") != string)
       return NULL;
    
    // Allocating len - 4 (sip:) +1
    int size = strlen(string)-3;
    char* ret = pkg_malloc(size);
    strncpy(ret,string+4,size-1);

    // Add the trailing \0
    ret[size-1] = '\0';
    LOG(L_INFO,"remove_sip_prefix(): returning -%s-\n",ret);
        
	return ret;
    
}

/* 
 * Route message to a given destination:
 *
 * FIXME: port is hardwired
 * 
 * Parameters:
 *
 * - sip_msg* msg, message to be routed
 * - char* dest, destination
 *
 * Returns:
 *
 * - 0, all rigth
 * - negative value, error condition
 * 
 */
static int route(struct sip_msg* msg, char* dest)
{

    // Allocate memory for two pointer of pointers
    void** param = pkg_malloc(2*sizeof(void**));
    param = &dest;
    *(param+1) = "5060";
    
    if (fixup_hostport2proxy(param,1)) {
        LOG(L_ERR,"route(): error fixing proxy/port (1)\n");
        return -1;
    }

    if (fixup_hostport2proxy(param+1,2)) {
        LOG(L_ERR,"route(): error fixing proxy/port (2)\n");
        return -2;
    }
    
    char* px = *param;
    char* foo = NULL;
    t_relay_to_udp_f(msg,px,foo);
    //t_relay_to_tcp_f(msg,px,foo);
   
    pkg_free(param);
    return 0;

}

/*
 * This function is intented to erase the first Route header if it's
 * matching myself.  *
 * Parameters:
 *
 * - sip_msg* msg, the SIP message
 *
 * Returns:
 *
 * - 0, all right, Route removed
 * - 1, no route to remove found
 * - 2, first route does not match with myself
 * - negative value, error condition
 * 
 */
static int remove_route_header(struct sip_msg* msg) {

    // Based on route_after_loose function in rr/loose.c
    
    // Find the first Route HF
    if ((find_first_route(msg) < 0)) {
        LOG(L_ERR,"remove_route_header(): No Route HF to remove found\n");
        return 1;
    }

    // To check if match with my own SIP URI
    struct sip_uri puri;
    str* uri;
    rr_t* rt;
    
    rt = (rr_t*) msg->route->parsed;
    uri = &rt->nameaddr.uri;
    
    if (parse_uri(uri->s, uri->len, &puri) < 0) {
        LOG(L_ERR,"remove_route_header(): Error while parsing the first route URI\n");
        return -1;
    }

#ifdef ENABLE_USER_CHECK
    if (is_myself(&puri.user, &puri.host, puri.port_no))
#else
    if (is_myself(&puri.host, puri.port_no))
#endif
    { 
        // The Route HF will be removed
        if (!del_lump(msg,msg->route->name.s - msg->buf, msg->route->len,0)) {
            LOG(L_ERR,"remove_route_header(): Can't remove Route HF\n");
            return -2;
        }
        
        LOG(L_INFO,"remove_route_header(): First Route HF matchs myself. Removing\n");
        return 0;
    }
    else {
        LOG(L_INFO,"remove_route_header(): First Route HF doesn't match myself. Not removing\n");
        return 2;
    }
}

/* 
 * We provide diferent functions to add the Route HF, mainly for debugging
 * purposes. This functions share the same prototype:
 *
 * Parameters:
 *
 * - sip_msg* msg, the SIP message
 * - char* header, the header to add
 *
 * Return:
 *
 * - 0, all right
 * - negative value, error condition
 * 
 * Modification in SIP messages are done using a 'lump'. See an
 * example of how to do it in modules/rr/record.c, function 
 * record_route_preset.
 *
 * RFC 3261 pp. 29 establishes, about the relative order of headers: 
 *
 * "The relative order of header fields with different field names is not
 * significant.  However, it is RECOMMENDED that header fields which are
 * needed for proxy processing (Via, Route, Record-Route, Proxy-Require,
 * Max-Forwards, and Proxy-Authorization, for example) appear towards
 * the top of the message to facilitate rapid parsing.  The relative
 * order of header field rows with the same field name is important.
 * Multiple header field rows with the same field-name MAY be present in
 * a message if and only if the entire field-value for that header field
 * is defined as a comma-separated list (that is, if follows the grammar
 * defined in Section 7.3).  It MUST be possible to combine the multiple
 * header field rows into one "field-name: field-value" pair, without
 * changing the semantics of the message, by appending each subsequent
 * field-value to the first, each separated by a comma.  The exceptions
 * to this rule are the WWW-Authenticate, Authorization, Proxy-
 * Authenticate, and Proxy-Authorization header fields.  Multiple header
 * field rows with these names MAY be present in a message, but since
 * their grammar does not follow the general form listed in Section 7.3,
 * they MUST NOT be combined into a single header field row."
 * 
 */
static int add_hf_before_all(struct sip_msg* msg, char* header) {
    
    struct lump* l;
    l = anchor_lump(msg, msg->headers->name.s - msg->buf, 0, 0);
    if (!l) {
        LOG(L_ERR, "add_hf_before_all(): Error while creating and anchor\n");
        return -1;
    }
    if (!insert_new_lump_after(l,header,strlen(header),0)) {
        LOG(L_ERR,"add_hf_before_all(): Error while inserting new lump\n");
        return -2;
    };

    LOG(LOG_INFO,"add_hf_before_all(): HF added OK\n");
    return 0;
}

static int add_hf_after_all(struct sip_msg* msg, char* header) {
    
   // Re-uses append_hf functionality (textops module)
    
    // Allocate memory for two pointer of pointers
    void** param = pkg_malloc(2*sizeof(void**));
    param = &header;
    
    if (str_fixup(param,1)) {
        LOG(L_ERR,"add_hf_afet_all(): Error fixing header\n");
        return -1;
    }

    str* h = *param;
    char* foo = NULL;
    append_hf_f(msg,h,foo);
   
    pkg_free(param);

    LOG(LOG_INFO,"add_hf_after_all(): HF added OK\n");
    return 0;
}

static int add_hf_after_max_forwards (struct sip_msg* msg, char* header) {

   	struct hdr_field *hf;
	struct lump* anchor;

    // It seems working, but I (Fermín) have doubts about the stability... SER seems
    // to crash more often than usual when using this
    
    // Make sure HDR_MAXFORWARDS header has been parsed
    parse_headers(msg, HDR_MAXFORWARDS, 0);
    hf = msg->maxforwards;
    int pos = hf->name.s - msg->buf;
    //int pos = msg->headers->name.s - msg->buf;
    anchor = anchor_lump(msg, pos, 0, 0);
	if (anchor == 0) {
	    LOG(L_ERR, "append_hf_after_max_forwards(): Can't get anchor\n");
		return -1;
	}
    if (insert_new_lump_before(anchor, header, strlen(header), 0) == 0) {
	    LOG(L_ERR, "append_hf_after_max_forwards(): Can't insert lump\n");
	    return -2;
    }

    LOG(LOG_INFO,"add_hf_after_max_forwards(): HF added OK\n");
    return 0;
    
}

static int add_hf_right_place (struct sip_msg* msg, char* header) {

    /* TODO: implement a function that add the Route header
     * in the 'right place'. I mean: if there is any other
     * Route HF (apart the one is removed using remove_route_header)
     * and close to the top of the message (for example, after the
     * 'Max-Fowards' header) 
     */
    
    LOG(LOG_INFO,"add_hf_right_place(): HF added OK\n");
    return 0;
}

/*
 * This function write a Route header based on the previously stored Path
 * value for the UE that is being INVITEd (or any other dialog-starting
 * transaction). The Request-URI sip address is used as key to search in
 * the data base.
 */
static int path_based_route(struct sip_msg* msg)
{

    // Step 1: search the Request-URI
    //
    // NOTE: in str.h is said that 's' is a 'null terminated string',
    // but test performed has demostrated (at least, for the str used
    // in request.uri) that is not true: strcpy magic is needed.
    
    char* request_uri = pkg_malloc(msg->first_line.u.request.uri.len + 1);
    if (!request_uri) {
        LOG(L_ERR,"path_based_route(): No memory left\n");
        return -1;
    }
    strncpy(request_uri,msg->first_line.u.request.uri.s,msg->first_line.u.request.uri.len);
    // Add the trailing \0
    request_uri[msg->first_line.u.request.uri.len] = '\0';

    if (request_uri == NULL) {
       LOG(L_ERR,"path_based_route(): NULL Request-URI? It seems a strange error...\n");
       pkg_free(request_uri);
       return -2;
    }

    char* request_uri2 = remove_sip_prefix(request_uri);
    if (request_uri2 == NULL) {
        LOG(L_INFO,"path_based_route(): Format error in string -%s-\n",request_uri);
        pkg_free(request_uri);
        return -3;
    }
    pkg_free(request_uri);

    LOG(L_INFO,"path_based_route(): URI to search for is: '%s'\n",request_uri2);

    // Step 2: search associated Path 
    
    char* stored_path = search_path(request_uri2);
    if (stored_path == NULL) {
        // No path has been found associated with this user
        LOG(L_INFO,"path_based_route(): No Path associated with '%s'!\n",request_uri2);
        pkg_free(request_uri2);

        // We return 0 in order to stop processing (if clause in
        // ser.cfg)
        LOG(L_INFO,"path_based_route(): Returning 0\n");
        return 0;
    }
    else {
        
        pkg_free(request_uri2);   // No longer needed
        
        // Step 3: add Route HF
        //
        // Note that stored_prefix is a "raw" SIP URI, doesn't include the
        // '<' and the ';lr>' tokens
        
        // Header composition
        char* prefix = "Route: <sip:";
        char* sufix = ";lr>";
        char* route_h = pkg_malloc(strlen(prefix)+strlen(stored_path)+strlen(sufix)+strlen(CRLF));
        if (!route_h) {
            LOG(L_ERR,"path_based_route(): No memory left\n");
            pkg_free(stored_path);
            return -4;
        }
                
        char* p = route_h;
        memcpy(p,prefix,strlen(prefix));
        p += strlen(prefix);
        memcpy(p,stored_path,strlen(stored_path));
        p += strlen(stored_path);
        memcpy(p,sufix,strlen(sufix));
        p += strlen(sufix);
        memcpy(p,CRLF,2);
        p += 2;
        
        LOG(L_INFO,"path_based_route(): The following header will be added: '%s'\n",route_h);
        //if (add_hf_before_all(msg,route_h) < 0) 
        if (add_hf_after_all(msg,route_h) < 0) {
        //if (add_hf_after_max_forwards(msg,route_h) < 0) 
            LOG(L_ERR, "path_based_route(): Error while adding Route HF\n");
            pkg_free(stored_path);
            pkg_free(route_h);
            return -5;
        }

        // Step 4: Remove Route HF if match myself
        // It is needed due to we are using (Step 6) t_realy_to function 
        // to route the message in path_based_route, overriding the standar loose 
        // routing treatment that would remove that Route HF.
        LOG(L_INFO,"path_based_route(): Deleting Route header...\n");
        if (remove_route_header(msg) < 0) {
            LOG(L_ERR,"path_based_route(): Error deleting Route header\n");
            pkg_free(stored_path);
            pkg_free(route_h);
            // FIXME: destroy the lump created with add_hf_* ?
            return -6;
        }
        
        // Step 5: Route to the SIP URI just added in the Route HF
        LOG(L_INFO,"path_based_route(): Routing the message...\n");
        if (route(msg,stored_path)) {
            LOG(L_ERR,"path_based_route(): Error routing the message\n");
            pkg_free(stored_path);
            pkg_free(route_h);
            // FIXME: destroy the lump created with add_hf_* ?
            return -7;
        }
        
        pkg_free(stored_path);
        // Not pkg_free(route_h)! It seems that the allocated memory
        // is removed by SER core when processing lumps in the sip_msg

        LOG(L_INFO,"path_based_route(): Returning 1\n");
        return 1;
    }
    
}

/*
 * This function stores the value of the Path header in the SIP message in
 * the data base
 * 
 * If the Path value does not exist in the message, the functions
 * perform no operation.
 * 
 */

static int store_path(struct sip_msg* msg)
{

    struct hdr_field *hf;

	parse_headers(msg, HDR_EOH, 0);
    int cnt = 0;
	for (hf=msg->headers; hf; hf=hf->next) {
        //LOG(L_INFO,"store_path: HF name: '%.*s'\n",hf->name.len,ZSW(hf->name.s));
        if (strncmp("Path",hf->name.s,hf->name.len) == 0) {
            LOG(L_INFO,"store_path(): Path found = '%.*s'\n",hf->body.len,ZSW(hf->body.s));
            cnt++;
            break;
        }
    }

    if (cnt == 0) {
        LOG(L_INFO,"store_path(): No Path HF found\n");
    }
    else {

        // We use 'From', 'To' would be used too. Make sure
        // the header has been parsed
        parse_headers(msg, HDR_FROM, 0);
        char* ue = pkg_malloc(msg->from->body.len + 1);
        if (!ue) {
            LOG(L_ERR,"store_path(): No memory left\n");
            return -1;
        }
        strncpy(ue,msg->from->body.s,msg->from->body.len);
        // Add the trailing \0
        ue[msg->from->body.len] = '\0';
        
        char* path = pkg_malloc(hf->body.len + 1);
        if (!path) {
            LOG(L_ERR,"store_path(): No memory left\n");
            pkg_free(ue);
            return -2;
        }
        strncpy(path,hf->body.s,hf->body.len);
        // Add the trailing \0
        path[hf->body.len] = '\0';
 
        // Clean path and ue
        LOG(L_INFO,"store_path(): Cleaning (%s,%s) before database storage\n",ue,path);

        char* path2 = remove_brackets(path);
        if (path2 == NULL) {
            LOG(L_INFO,"store_path(): Format error in string -%s-\n",path);
            pkg_free(ue);
            pkg_free(path);
            return -3;
        }
        pkg_free(path);
        char* path3 = remove_user_prefix(path2);
        if (path3 == NULL) {
            LOG(L_INFO,"store_path(): Format error in string -%s-\n",path2);
            pkg_free(ue);
            pkg_free(path2);
            return -4;
        }
        pkg_free(path2);
        char* path4 = remove_lr_sufix(path3);
        if (path4 == NULL) {
            LOG(L_INFO,"store_path(): Format error in string -%s-\n",path3);
            pkg_free(ue);
            pkg_free(path3);
            return -5;
        }
        pkg_free(path3);
     
        char* ue2 = remove_brackets(ue);
        if (ue2 == NULL) {
            LOG(L_INFO,"store_path(): Format error in string -%s-\n",ue);
            pkg_free(ue);
            pkg_free(path4);
            return -6;
        }
        pkg_free(ue);

        // Database storage
        LOG(L_INFO,"store_path(): Storing (%s,%s)\n",ue2,path4);
        if (write_path(ue2,path4) < 0) {
            LOG(L_ERR,"store_path(): Error storing path\n");
            pkg_free(ue2);
            pkg_free(path4);
            return -7;
        }

        pkg_free(ue2);
        pkg_free(path4);
        
    }

    LOG(L_INFO,"store_path(): Returning 1\n");
    return 1;
  
}

/* For testing and debugging purposes only */
static int test_header(struct sip_msg* msg)
{

        // Header composition
        char* prefix = "X-Path-Test: This is a test!";
        char* test_h = pkg_malloc(strlen(prefix)+strlen(CRLF));
        if (!test_h) {
            LOG(L_ERR,"test_header(): No memory left\n");
            return -1;
        }
                
        char* p = test_h;
        memcpy(p,prefix,strlen(prefix));
        p += strlen(prefix);
        memcpy(p,CRLF,2);
        p += 2;
        
        LOG(L_INFO,"test_header(): The following header will be added: '%s'\n",test_h);
    
        struct lump* l;
        l = anchor_lump(msg, msg->headers->name.s - msg->buf, 0, 0);
        if (!l) {
            LOG(L_ERR, "test_header(): Error while creating and anchor\n");
            pkg_free(test_h);
            return -2;
        }
        if (!insert_new_lump_after(l,test_h,strlen(test_h),0)) {
            LOG(L_ERR,"test_header(): Error while inserting new lump\n");
            pkg_free(test_h);
            return -3;
        };

        LOG(L_ERR,"test_header(): Returning 1\n");
        return 1;

}

/* For testing and debugging purposes only */
static int test_routing(struct sip_msg* msg)
{

    // Allocate memory for two pointer of pointers
    void** param = pkg_malloc(2*sizeof(void**));
    *param = "pcscf.domain2.com";
    *(param+1) = "5060";
    
    if (fixup_hostport2proxy(param,1)) {
        LOG(L_ERR,"test_routing(): error fixing proxy/port (1)\n");
        return -1;
    }

    if (fixup_hostport2proxy(param+1,2)) {
        LOG(L_ERR,"test_routing(): error fixing proxy/port (2)\n");
        return -2;
    }
    
    char* px = *param;
    char* foo = NULL;
    t_relay_to_udp_f(msg,px,foo);
    //t_relay_to_tcp_f(msg,px,foo);
   
    pkg_free(param);
    return 1;

}


static int mod_init(void)
{

    fprintf(stderr, "%s - initializing\n", exports.name);
    LOG(L_ALERT, "WARNING! This module is experimental and may crash SER or create unexpected results. Use it at your own risk!");
    
    // Create path_db file
    int fd;
    if ( (fd = creat(db_file, 0666)) < 1 ) {
        LOG(L_ERR,"PATH: init_mod: cann't open path_db file: %s\n",db_file);
        return -1;
    }
    close (fd);

    // Fill up function pointers
    if (!(t_relay_to_udp_f = find_export("t_relay_to_udp",2,0))) {
        return -1;
    }
    if (!(t_relay_to_tcp_f = find_export("t_relay_to_tcp",2,0))) {
        return -1;
    }
    if (!(append_hf_f = find_export("append_hf",1,0))) {
        return -1;
    }
   
    return 0;    
}

static void destroy (void) {

    // Remove path_db file
    if (unlink(db_file) != 0) {
        LOG(L_ERR,"PATH: destroy: error removing path_db file:%s\n",db_file);
    }
}

