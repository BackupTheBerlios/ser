/* 
 * $Id: contact.c,v 1.10 2002/08/08 18:17:58 janakj Exp $ 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contact.h"
#include "../../str.h"
#include "utils.h"
#include "../../dprint.h"
#include "../../mem/mem.h"
#include "defs.h"
#include "sh_malloc.h"
#include "usrloc.h"

/*
 * Print contact, for debugging purposes only
 */
void print_contact(contact_t* _c)
{
#ifdef PARANOID
	if (!_c) {
		LOG(L_ERR, "print_contact(): Invalid parameter value\n");
		return;
	}
#endif
	LOG(L_ERR, "    Contact=\"%s\" expires=%d q=%-3.2f, Call-ID=%s CSeq=%d\n",
	       _c->c.s, (unsigned int)_c->expires, _c->q, _c->callid, _c->cseq);
}


/*
 * Create a new contact structure
 */
int create_contact(contact_t** _con, str* _aor, const char* _c, time_t _expires, float _q,
		   const char* _callid, int _cseq)
{
	int len;
#ifdef PARANOID
	if ((!_con) || (!_aor) || (!_c) || (!_callid)) {
		LOG(L_ERR, "create_contact(): Invalid parameter value\n");
		return FALSE;
	}
#endif

	*_con = (contact_t*)sh_malloc(sizeof(contact_t));
	if (!(*_con)) {
	        LOG(L_ERR, "create_contact(): No memory left\n");
		return FALSE;
	}

	len = strlen(_c);
	(*_con)->c.s = (char *)sh_malloc(len + 1);
	if (!((*_con)->c.s)) {
		LOG(L_ERR, "create_contact(): No memory left\n");
		sh_free(*_con);
		return FALSE;
	}

	memcpy((*_con)->c.s, _c, len + 1);
	strlower((*_con)->c.s, len);
        (*_con)->c.len = len;

	(*_con)->aor = _aor;
	(*_con)->expires = _expires;
	(*_con)->q = _q;

	len = strlen(_callid);
	(*_con)->callid = (char*)sh_malloc(len + 1);
	if (!((*_con)->callid)) {
		LOG(L_ERR, "create_contact(): No memory left\n");
		sh_free(*_con);
		sh_free((*_con)->c.s);
		return FALSE;
	}
	memcpy((*_con)->callid, _callid, len + 1);
	(*_con)->cseq = _cseq;
	(*_con)->next = NULL;

	return TRUE;
}	


/*
 * Free all memory associated with given contact structure
 */
void free_contact(contact_t* _c)
{
	if (_c) {
		sh_free(_c->callid);
		sh_free(_c->c.s);
		sh_free(_c);
	}
}


int update_contact(contact_t* _dst, contact_t* _src)
{
	char* ptr;
	int len;
#ifdef PARANOID
	if ((!_dst) || (!_src)) {
		LOG(L_ERR, "update_contact(): Invalid parameter value\n");
		return FALSE;
	}

	if (_src->expires == 0) {
		LOG(L_ERR, "update_contact(): Binding should be removed, not updated\n");
		return FALSE;
	}
#endif

	len = strlen(_src->callid);
	ptr = (char*)sh_malloc(len + 1);
	if (!ptr) {
		LOG(L_ERR, "update_contact(): No memory left\n");
		return FALSE;
	}
	memcpy(ptr, _src->callid, len + 1);

	sh_free(_dst->callid);
	_dst->callid = ptr;

	_dst->expires = _src->expires;
	_dst->q = _src->q;
	_dst->cseq = _src->cseq;

	return TRUE;
}



/*
 * Compare contacts
 */
int cmp_contact(contact_t* _c1, contact_t* _c2)
{
	if (!strcmp(_c1->c.s, _c2->c.s)) {
		return TRUE;
	} else {
		return FALSE;
	}
}



int db_remove_contact(db_con_t* _c, contact_t* _con)
{
	db_key_t keys[2] = {user_col, contact_col};
	db_val_t vals[2] = {{DB_STRING, 0, {.string_val = NULL}},
			    {DB_STRING, 0, {.string_val = NULL}}
	};

#ifdef PARANOID
	if ((!_c) || (!_con)) {
		LOG(L_ERR, "db_remove_contact(): Invalid parameter value\n");
		return FALSE;
	}
#endif

	vals[0].val.string_val = _con->aor->s;
	vals[1].val.string_val = _con->c.s;

	if (db_delete(_c, keys, vals, 2) < 0) {
		LOG(L_ERR, "db_remove_contact(): Error while deleting from database\n");
		return FALSE;
	}

	return TRUE;
}



int db_update_contact(db_con_t* _c, contact_t* _con)
{
	db_key_t keys1[2] = {user_col, contact_col};
	db_val_t vals1[2] = {{DB_STRING, 0, {.string_val = NULL}},
			    {DB_STRING, 0, {.string_val = NULL}}
	};

	db_key_t keys2[4] = {expires_col, q_col, callid_col, cseq_col};
	db_val_t vals2[4] = {{DB_DATETIME, 0, {.time_val = 0}},
			     {DB_DOUBLE, 0, {.double_val = 0}},
			     {DB_STRING, 0, {.string_val = NULL}},
			     {DB_INT, 0, {.int_val = 0}}
	};

#ifdef PARANOID
	if ((!_c) || (!_con)) {
		LOG(L_ERR, "db_remove_contact(): Invalid parameter value\n");
		return FALSE;
	}
#endif
	vals1[0].val.string_val = _con->aor->s;
	vals1[1].val.string_val = _con->c.s;

	vals2[0].val.time_val = _con->expires;
	vals2[1].val.double_val = _con->q;
	vals2[2].val.string_val = _con->callid;
	vals2[3].val.int_val = _con->cseq;

	if (db_update(_c, keys1, vals1, keys2, vals2, 2, 4) < 0) {
		LOG(L_ERR, "db_update_contact(): Error while updating binding\n");
		return FALSE;
	}

	return TRUE;
}




int db_insert_contact(db_con_t* _c, contact_t* _con)
{
	db_key_t keys[] = {user_col, contact_col, expires_col, q_col, callid_col, cseq_col};
	db_val_t vals[] = {{DB_STRING,   0, {.string_val = NULL}},
			  {DB_STRING,   0, {.string_val = NULL}},
			  {DB_DATETIME, 0, {.time_val = 0}},
			  {DB_DOUBLE,   0, {.double_val = 0}},
			  {DB_STRING,   0, {.string_val = NULL}},
			  {DB_INT,      0, {.int_val = 0}}
	};

#ifdef PARANOID
	if (!_con) {
		LOG(L_ERR, "db_insert_contact(): Invalid parameter value\n");
		return FALSE;
	}

#endif
	vals[0].val.string_val = _con->aor->s;
	vals[1].val.string_val = _con->c.s;
	vals[2].val.time_val = _con->expires;
	vals[3].val.double_val = _con->q;
	vals[4].val.string_val = _con->callid;
	vals[5].val.int_val = _con->cseq;
	
	if (db_insert(_c, keys, vals, 6) < 0) {
		LOG(L_ERR, "db_insert_contact(): Error while inserting binding\n");
		return FALSE;
	}

	return TRUE;
}

