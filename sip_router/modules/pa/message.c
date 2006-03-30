#include "pa_mod.h"
#include "message.h"
#include "../../id.h"
#include "../../parser/parse_from.h"
#include <cds/sstr.h>

#include <xcap/msg_rules.h>

static int xcap_get_msg_rules(str *uid, 
		msg_rules_t **dst, char *xcap_root)
{
	xcap_query_params_t xcap;
	int res;
	/* str u; */
	
	/* get only presentity name, not whole uri
	 * can't use parse_uri because of absence 
	 * of protocol specification ! */
	/* if (get_user_from_uri(uri, &u) != 0) u = *uri; */

	DBG("querying IM-authorization document from XCAP server: %s\n",
			xcap_root);
	
	memset(&xcap, 0, sizeof(xcap));
	/* TODO: 
	xcap.auth_user = "???";
	xcap.auth_pass = "???"; */
	xcap.enable_unverified_ssl_peer = 1;
	res = get_msg_rules(xcap_root, uid, &xcap, dst);
	return res;
}

static int get_sender_uri(struct sip_msg* _m, str* uri)
{
	struct sip_uri puri;
	int res = 0;
	
	uri->s = get_from(_m)->uri.s;
	uri->len = get_from(_m)->uri.len;

	if (parse_uri(uri->s, uri->len, &puri) < 0) {
		LOG(L_ERR, "Error while parsing URI\n");
		return -1;
	}
	
	uri->s = puri.user.s;
	if ((!uri->s) || (puri.user.len < 1)) {
		uri->s = puri.host.s;
		uri->len = puri.host.len;
		res = 1; /* it is uri without username ! */
	}
	uri->len = puri.host.s + puri.host.len - uri->s;
	return res;
}

int authorize_message(struct sip_msg* _m, char* _xcap_root, char*_st)
{
	/* get and process XCAP authorization document */
	/* may modify the message or its body */

	str uid = STR_NULL;
	msg_rules_t *rules = NULL;
	msg_handling_t mh = msg_handling_allow;
	str sender_uri = STR_NULL;
	
	get_sender_uri(_m, &sender_uri);
	
	if (get_to_uid(&uid, _m) < 0) {
		ERR("get_to_uid failed\n");
		/* enabled */
		return 1;
	}
	
	if (xcap_get_msg_rules(&uid, &rules, _xcap_root) < 0) {
		/* enabled */
		TRACE("get_msg_rules failed\n");
		return 1;
	}
	
	if (get_msg_rules_action(rules, &sender_uri, &mh) != 0)
		mh = msg_handling_allow;

	free_msg_rules(rules);

	switch (mh) {
		case msg_handling_block: 
			DBG("XCAP AUTH MESSAGE: block\n");
			return -1;
		case msg_handling_allow: 
			DBG("XCAP AUTH MESSAGE: allow\n");
			return 1;
	}
	
	return -1;
}
