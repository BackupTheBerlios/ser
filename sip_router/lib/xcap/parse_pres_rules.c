/* 
 * Copyright (C) 2005 iptelorg GmbH
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

#include <xcap/parse_pres_rules.h>
#include <xcap/xcap_result_codes.h>

#include <cds/dstring.h>
#include <cds/memory.h>
#include <cds/logger.h>
#include <cds/list.h>
#include <string.h>

#include <xcap/xml_utils.h>

char *common_policy_ns = NULL;
char *pres_rules_ns = NULL;

static cp_unknown_t *create_unknown(int data_size)
{
	cp_unknown_t *u = cds_malloc(sizeof(cp_unknown_t) + data_size);
	u->next = NULL;
	return u;
}

static int str2sub_handling(const char *s, sub_handling_t *dst)
{
	if (!s) return RES_INTERNAL_ERR;
	
	if (strcmp(s, "allow") == 0) {
		*dst = sub_handling_allow;
		return 0;
	}
	if (strcmp(s, "block") == 0) {
		*dst = sub_handling_block;
		return 0;
	}
	if (strcmp(s, "polite-block") == 0) {
		*dst = sub_handling_polite_block;
		return 0;
	}
	if (strcmp(s, "confirm") == 0) {
		*dst = sub_handling_confirm;
		return 0;
	}
	ERROR_LOG("invalid sub-handling value: \'%s\'\n", s);
	return RES_INTERNAL_ERR;
}

static int read_actions(xmlNode *an, cp_actions_t **dst)
{
	xmlNode *n;
	const char *s;
	int res = RES_OK;
	if ((!an) || (!dst)) return RES_INTERNAL_ERR;
	
	*dst = (cp_actions_t*)cds_malloc(sizeof(cp_actions_t));
	if (!dst) return RES_MEMORY_ERR;
	memset(*dst, 0, sizeof(cp_actions_t));

	n = find_node(an, "sub-handling", common_policy_ns);
	if (n) {
		/* may be only one sub-handling node? */
		s = get_node_value(n);
		(*dst)->unknown = create_unknown(sizeof(sub_handling_t));
		if (!(*dst)->unknown) return RES_MEMORY_ERR;
		res = str2sub_handling(s, (sub_handling_t*)(*dst)->unknown->data);
	}

	return res;
}

static int read_sphere(xmlNode *n, cp_sphere_t **dst)
{
	*dst = (cp_sphere_t*)cds_malloc(sizeof(cp_sphere_t));
	if (!*dst) return RES_MEMORY_ERR;
	(*dst)->next = NULL;
	
	str_dup_zt(&(*dst)->value, get_node_value(n));
	return RES_OK;
}

static int read_validity(xmlNode *n, cp_validity_t **dst)
{
	const char *from, *to;
	*dst = (cp_validity_t*)cds_malloc(sizeof(cp_validity_t));
	if (!*dst) return RES_MEMORY_ERR;
	
	from = get_node_value(find_node(n, "from", common_policy_ns));
	to = get_node_value(find_node(n, "to", common_policy_ns));
	
	(*dst)->from = xmltime2time(from);
	(*dst)->to = xmltime2time(to);
	return RES_OK;
}

static int read_id(xmlNode *n, cp_id_t **dst)
{
	*dst = (cp_id_t*)cds_malloc(sizeof(cp_id_t));
	if (!*dst) return RES_MEMORY_ERR;
	(*dst)->next = NULL;
	
	get_str_attr(n, "entity", &(*dst)->entity);
	if ((*dst)->entity.len == 0) {
		/* hack - eyeBeams format differs from draft ! */
		str_dup_zt(&(*dst)->entity, get_node_value(n));
	}

	return RES_OK;
}

static int read_domain(xmlNode *n, cp_domain_t **dst)
{
	*dst = (cp_domain_t*)cds_malloc(sizeof(cp_domain_t));
	if (!*dst) return RES_MEMORY_ERR;
	(*dst)->next = NULL;
	
	get_str_attr(n, "domain", &(*dst)->domain);
	return RES_OK;
}

static int read_except(xmlNode *n, cp_except_t **dst)
{
	*dst = (cp_except_t*)cds_malloc(sizeof(cp_except_t));
	if (!*dst) return RES_MEMORY_ERR;
	(*dst)->next = NULL;
	
	get_str_attr(n, "entity", &(*dst)->entity);
	return RES_OK;
}

static int read_except_domain(xmlNode *n, cp_except_domain_t **dst)
{
	*dst = (cp_except_domain_t*)cds_malloc(sizeof(cp_except_domain_t));
	if (!*dst) return RES_MEMORY_ERR;
	(*dst)->next = NULL;
	
	get_str_attr(n, "domain", &(*dst)->domain);
	return RES_OK;
}

static int read_any_identity(xmlNode *an, cp_any_identity_t **dst)
{
	cp_domain_t *domain, *last_domain = NULL;
	cp_except_domain_t *except, *last_except = NULL;
	xmlNode *n;
	int res = RES_OK;
	
	*dst = (cp_any_identity_t*)cds_malloc(sizeof(cp_any_identity_t));
	if (!*dst) return RES_MEMORY_ERR;
	
	n = an->children;
	while (n) {
		if (n->type == XML_ELEMENT_NODE) {
			if (cmp_node(n, "domain", common_policy_ns) >= 0) {
				res = read_domain(n, &domain);
				if (res != 0) break;
				LINKED_LIST_ADD((*dst)->domains, last_domain, domain);
			}
			else if (cmp_node(n, "except-domain", common_policy_ns) >= 0) {
				res = read_except_domain(n, &except);
				if (res != 0) break;
				LINKED_LIST_ADD((*dst)->except_domains, last_except, except);
			}
		}
		
		n = n->next;
	}
	return res;
}

static int read_identity(xmlNode *idn, cp_identity_t **dst)
{
	cp_id_t *id, *last_id = NULL;
	cp_domain_t *domain, *last_domain = NULL;
	cp_except_t *except, *last_except = NULL;
	xmlNode *n;
	int res = RES_OK;
	
	*dst = (cp_identity_t*)cds_malloc(sizeof(cp_identity_t));
	if (!*dst) return RES_MEMORY_ERR;
	memset(*dst, 0, sizeof(**dst));

	n = idn->children;
	while (n) {
		if (n->type == XML_ELEMENT_NODE) {
			if (cmp_node(n, "id", common_policy_ns) >= 0) {
				res = read_id(n, &id);
				if (res != 0) break;
				LINKED_LIST_ADD((*dst)->ids, last_id, id);
			}
			else if (cmp_node(n, "domain", common_policy_ns) >= 0) {
				res = read_domain(n, &domain);
				if (res != 0) break;
				LINKED_LIST_ADD((*dst)->domains, last_domain, domain);
			}
			else if (cmp_node(n, "except", common_policy_ns) >= 0) {
				res = read_except(n, &except);
				if (res != 0) break;
				LINKED_LIST_ADD((*dst)->excepts, last_except, except);
			}
			else if (cmp_node(n, "any-identity", common_policy_ns) >= 0) {
				res = read_any_identity(n, &(*dst)->any_identity);
				if (res != 0) break;
			}
		}
		
		n = n->next;
	}
	
	return res;
}

static int read_conditions(xmlNode *cn, cp_conditions_t **dst)
{
	xmlNode *n;
	int res = RES_OK;
	cp_sphere_t *sphere, * last_sphere = NULL;
	if ((!cn) || (!dst)) return RES_INTERNAL_ERR;
	
	*dst = (cp_conditions_t*)cds_malloc(sizeof(cp_conditions_t));
	if (!dst) return RES_MEMORY_ERR;
	memset(*dst, 0, sizeof(cp_conditions_t));
	
	n = cn->children;
	while (n) {
		if (n->type == XML_ELEMENT_NODE) {
			if (cmp_node(n, "validity", common_policy_ns) >= 0) {
				/* FIXME: free existing validity */
				res = read_validity(n, &(*dst)->validity);
				if (res != 0) break;
			}
			else {
				if (cmp_node(n, "identity", common_policy_ns) >= 0) {
					/* FIXME: free existing identity */
					res = read_identity(n, &(*dst)->identity);
					if (res != 0) break;
				}
				else {
					if (cmp_node(n, "sphere", common_policy_ns) >= 0) {
						res = read_sphere(n, &sphere);
						if (res != 0) break;
						LINKED_LIST_ADD((*dst)->spheres, last_sphere, sphere);
					}
					/* else process other elements ? */
				}
				
			}
		}
		n = n->next;
	}

	return res;
}

static int read_transformations(xmlNode *tn, cp_transformations_t **dst)
{
	int res = RES_OK;
	if ((!tn) || (!dst)) return RES_INTERNAL_ERR;
	
	*dst = (cp_transformations_t*)cds_malloc(sizeof(cp_transformations_t));
	if (!dst) return RES_MEMORY_ERR;
	memset(*dst, 0, sizeof(cp_transformations_t));

	DEBUG_LOG("transformations for pres_rules not used\n");

	return res;
}

static int read_rule(xmlNode *rn, cp_rule_t **dst)
{
	xmlNode *n;
	int res = RES_OK;
	if ((!rn) || (!dst)) return RES_INTERNAL_ERR;
	
	*dst = (cp_rule_t*)cds_malloc(sizeof(cp_rule_t));
	if (!dst) return RES_MEMORY_ERR;
	memset(*dst, 0, sizeof(cp_rule_t));

	get_str_attr(rn, "id", &(*dst)->id);

	n = find_node(rn, "actions", common_policy_ns);
	if (n) {
		res = read_actions(n, &(*dst)->actions);
		if (res != 0) return res;
	}
	
	n = find_node(rn, "conditions", common_policy_ns);
	if (n) {
		res = read_conditions(n, &(*dst)->conditions);
		if (res != 0) return res;
	}
	
	n = find_node(rn, "transformations", common_policy_ns);
	if (n) {
		res = read_transformations(n, &(*dst)->transformations);
		if (res != 0) return res;
	}

	return res;
}

static int read_pres_rules(xmlNode *root, cp_ruleset_t **dst)
{
	cp_ruleset_t *rs = NULL;
	cp_rule_t *r, *last = NULL;
	xmlNode *n;
	int res = RES_OK;
	
	if (!dst) return RES_INTERNAL_ERR;
	else *dst = NULL;
	if (!root) return RES_INTERNAL_ERR;
	
	if (cmp_node(root, "ruleset", common_policy_ns) < 0) {
		ERROR_LOG("document is not a ruleset \n");
		return RES_INTERNAL_ERR;
	}

	rs = (cp_ruleset_t*)cds_malloc(sizeof(cp_ruleset_t));
	if (!rs) return RES_MEMORY_ERR;
	*dst = rs;
	memset(rs, 0, sizeof(*rs));

	
	/* read rules in ruleset */
	n = root->children;
	while (n) {
		if (n->type == XML_ELEMENT_NODE) {
			if (cmp_node(n, "rule", common_policy_ns) >= 0) {
				res = read_rule(n, &r);
				if (res == 0) {
					if (r) LINKED_LIST_ADD(rs->rules, last, r);
				}
				else break;
			}
		}
		n = n->next;
	}

	return res;
}

int parse_pres_rules(const char *data, int dsize, cp_ruleset_t **dst)
{
	int res = 0;
	xmlDocPtr doc; /* the resulting document tree */

	if (dst) *dst = NULL;
	doc = xmlReadMemory(data, dsize, NULL, NULL, xml_parser_flags);
	if (doc == NULL) {
		ERROR_LOG("can't parse document\n");
		return RES_INTERNAL_ERR;
	}
	
	res = read_pres_rules(xmlDocGetRootElement(doc), dst);
	if ((res != RES_OK) && (dst)) {
		/* may be set => must be freed */
		free_pres_rules(*dst);
		*dst = NULL;
	}

	xmlFreeDoc(doc);
	return res;
}
