/*
 * $Id: snmp_handler.h,v 1.2 2002/09/19 12:23:55 jku Rel $
 *
 * SNMP Module: Header file for dynamic handler registration.
 * NOTE: In separate file because both the module and the snmp agent
 * need this, but the snmp headers conflict with ser headers needed
 * by the module
 *
 * Copyright (C) 2001-2003 Fhg Fokus
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


/* The Idea:
 * When a table handler is called, it looks up its internal handler
 * table (indexed by column number) to see if the particular 
 * requested object has a handler registered. If it does, it'll call
 * it. If it doesn't nothing happens.
 * When snmp_register_handler() is called, the appropiate table must
 * be found and the passed handler added to it. 
 *
 * NOTE: Should we add all possible rows with dummy default values upon
 * startup or as handlers are registered we update the rows with
 * default value? For now, the row is updated upon handler registration.
 * Good thing about this is that the table handler won't be called
 * for objects which we don't support (i.e. haven't been registered)
 */

#ifndef _SNMP_HANDLER_H_
#define _SNMP_HANDLER_H_

/* The types. SNMP #includes conflict with ser #include's so here we
 * provide a mapping for ASN1 types */
extern const u_char ser_types[];
enum ser_types { SER_INTEGER, SER_INTEGER32, SER_UNSIGNED, SER_TIMETICKS,
	SER_GAUGE, SER_COUNTER, SER_ENUMVAL, SER_STRING, SER_BITSTRING,
	SER_OBJID };

struct sip_snmp_obj {
	struct sip_snmp_obj *next;
	unsigned int col;	/* the column (last digit in the oid) */
	unsigned int row;	/* the row.. tricky if there's more than 2 indexes */
	enum ser_types type;		/* enum ser_types */
	/* The value which should be reported to SNMP. Upon handler registration,
	 * pass here the default value. If there's no default value for your
	 * object (e.g. because you need to compute it every time its needed) 
	 * pass 0 or "" */
	union {	
		void	*voidp;
		long	*integer;
		u_char	*string;
	} value;
	size_t	val_len;
	/* A placeholder for additional data that may be needed in some situations,
	 * but that can have different uses.
	 * For now is used by global table handlers to get a pointer
	 * to a function they can use to (re)fill a specific row in a table
	 */
	void *opaque;
};

/* The SNMP handlers for a particular object. Should return -1 on error.
 * On GET, it can return 0 if it wants to pass a value back up, or 1
 * if not. This is useful for generic handlers (e.g. table handlers)
 * which sometimes may not need to change a value put in the table upon
 * registration.
 * - On GET the passed structure may or may not have space allocated for the 
 *   real value. Check for NULL'ness. But even if it's not NULL, you may
 *   need to reallocate its space to fit your object (be careful specially
 *   with strings)
 * - On SET it doesn't need to check the type field, since we do that before, 
 *   based on the value stored during registration.
 */
enum handler_op { SER_GET, SER_SET, SER_UNDO, SER_COMMIT };
typedef int(*sip_handler)(struct sip_snmp_obj*, enum handler_op);

/* Structure defining the handlers for a particular SNMP object. If a
 * particular operation is not supported by the object (by definition or
 * by implementation) just make the corresponding field NULL. The last two 
 * are needed for SET processing, since in the event of a multi-SET command,
 * if one of them fails, all should fail (in the sense that it should be
 * as if nothing had happened) */
struct sip_snmp_handler {
	/* the object */
	struct sip_snmp_obj	*sip_obj;
	/* the functions */
	sip_handler			on_get;
	sip_handler			on_set;
	sip_handler			on_end;	/* undo or commit, look at op arg */
	struct sip_snmp_handler *next;
};

int handler_init();

/* a function that registers a handler with a particular table.
 * The column is passed inside the handler structure. The second argument
 * is the operation to perform. Should return -1 on error (that the passed
 * operation is not supported is considered an error) */
#define REG_OBJ 1	/* register object handler */
#define REG_TABLE 2	/* register table handler */
#define NEW_ROW 3	/* create row */
typedef int(*reg_handler)(struct sip_snmp_handler*, int op);

/* used by everybody in ser to register their SNMP objects and handlers
 * with us */
int snmp_register_handler(const char *name, struct sip_snmp_handler *h);
int snmp_register_row(const char *table, struct sip_snmp_handler *h);
int snmp_register_table(const char *table, struct sip_snmp_handler *h);

struct sip_snmp_handler* snmp_new_handler(size_t val_len);
struct sip_snmp_obj* snmp_new_obj(enum ser_types type, void *value,
		size_t val_len);
/* frees the value, the sip_obj and the handler */
void snmp_free_handler(struct sip_snmp_handler *h);
void snmp_free_obj(struct sip_snmp_obj *o);

/* doesn't clone the value */
struct sip_snmp_handler* snmp_clone_handler(struct sip_snmp_handler*);

#endif
