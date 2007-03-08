/*
 * $Id: auth_http.c,v 1.2 2007/03/08 16:27:41 gkovacs Exp $ 
 *
 * Copyright (c) 2007 iptelorg GmbH
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/bio.h>


#include "../../mem/mem.h"
#include "../../data_lump.h"

#include "auth_identity.h"

size_t curlmem_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t irealsize = size * nmemb;

	/* too big certificate */
	if (((str*)data)->len + irealsize >= CERTIFICATE_LENGTH)
		return 0;

	memcpy(&(((str*)data)->s[((str*)data)->len]), ptr, irealsize);
	((str*)data)->len+=irealsize;

	return irealsize;
}

int download_cer(str *suri, CURL *hcurl)
{
	CURLcode iRes;
	long lerr=200;
	char snulled[CERTIFICATE_URL_LENGTH], *snulledptr=NULL;
	int iRet=0;

	if (suri->len < sizeof(snulled)) {
		memcpy(snulled, suri->s, suri->len);
		snulled[suri->len]=0;
	} else {
		/* +1 for the terminating \0 byte */
		if (!(snulledptr=pkg_malloc(suri->len + 1))) {
			LOG(L_ERR, "AUTH_IDENTITY:download_cer: Not enough memory error\n");
			return -1;
		}
		memcpy(snulledptr, suri->s, suri->len);
		snulledptr[suri->len]=0;
	}

	do {
		if ((iRes=curl_easy_setopt(hcurl,
								   CURLOPT_URL,
								   snulledptr ? snulledptr : snulled))!=0) {
			LOG(L_ERR,
				"AUTH_IDENTITY:download_cer: Unable to set the url of certificate: %s\n",
				curl_easy_strerror(iRes));
			iRet=-2;
			break;
		}

		if ((iRes=curl_easy_perform(hcurl))!=0) {
			LOG(L_ERR,
				"AUTH_INDENTITY:download_cer: Error while downloading certificate '%s'\n",
				curl_easy_strerror(iRes));
			iRet=-3;
			break;
		}

		curl_easy_getinfo(hcurl,CURLINFO_RESPONSE_CODE,&lerr);
		if (lerr/200 != 1) {
			LOG(L_ERR, "AUTH_INDENTITY:download_cer: Bad HTTP response: %ld\n", lerr );
			iRet=-4;
		}
	} while (0);

	if (snulledptr)
		pkg_free(snulledptr);

	return iRet;
}
