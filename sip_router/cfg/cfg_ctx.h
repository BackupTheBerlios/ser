/*
 * $Id: cfg_ctx.h,v 1.4 2008/01/24 15:36:56 tirpi Exp $
 *
 * Copyright (C) 2007 iptelorg GmbH
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
 *
 * History
 * -------
 *  2007-12-03	Initial version (Miklos)
 */

#ifndef _CFG_CTX_H
#define _CFG_CTX_H

#include "../str.h"
#include "../locking.h"
#include "cfg.h"
#include "cfg_struct.h"

/* linked list of variables with their new values */
typedef struct _cfg_changed_var {
	cfg_group_t	*group;
	cfg_mapping_t	*var;
	struct _cfg_changed_var	*next;

	/* blob that contains the new value */
	unsigned char	new_val[1];
} cfg_changed_var_t;

/* callback that is called when a new group is declared */
typedef void (*cfg_on_declare)(str *, cfg_def_t *);

/* linked list of registered contexts */
typedef struct _cfg_ctx {
	/* variables that are already changed
	but have not been committed yet */
	cfg_changed_var_t	*changed_first;
	cfg_changed_var_t	*changed_last;
	/* lock potecting the the linked-list of
	changed variables */
	gen_lock_t		lock;

	/* callback that is called when a new
	group is registered */
	cfg_on_declare		on_declare_cb;

	struct _cfg_ctx	*next;
} cfg_ctx_t;

#define CFG_CTX_LOCK(ctx)	lock_get(&(ctx)->lock)
#define CFG_CTX_UNLOCK(ctx)	lock_release(&(ctx)->lock)

/* creates a new config context that is an interface to the
 * cfg variables with write permission */
cfg_ctx_t *cfg_register_ctx(cfg_on_declare on_declare_cb);

/* free the memory allocated for the contexts */
void cfg_ctx_destroy(void);

/* set the value of a variable without the need of explicit commit */
int cfg_set_now(cfg_ctx_t *ctx, str *group_name, str *var_name,
			void *val, unsigned int val_type);
int cfg_set_now_int(cfg_ctx_t *ctx, str *group_name, str *var_name, int val);
int cfg_set_now_string(cfg_ctx_t *ctx, str *group_name, str *var_name, char *val);

/* sets the value of a variable but does not commit the change */
int cfg_set_delayed(cfg_ctx_t *ctx, str *group_name, str *var_name,
			void *val, unsigned int val_type);
int cfg_set_delayed_int(cfg_ctx_t *ctx, str *group_name, str *var_name, int val);
int cfg_set_delayed_string(cfg_ctx_t *ctx, str *group_name, str *var_name, char *val);

/* commits the previously prepared changes within the context */
int cfg_commit(cfg_ctx_t *ctx);

/* drops the not yet committed changes within the context */
int cfg_rollback(cfg_ctx_t *ctx);

/* returns the value of a variable */
int cfg_get_by_name(cfg_ctx_t *ctx, str *group_name, str *var_name,
			void **val, unsigned int *val_type);

/* returns the description of a variable */
int cfg_help(cfg_ctx_t *ctx, str *group_name, str *var_name,
			char **ch, unsigned int *input_type);

/* notify the drivers about the new config definition */
void cfg_notify_drivers(char *group_name, int group_name_len, cfg_def_t *def);

/* initialize the handle for cfg_get_group_next() */
#define cfg_get_group_init(handle) \
	(*(handle)) = (void *)cfg_group

/* returns the group name and the cfg structure definition,
 * and moves the handle to the next group
 * Return value:
 *	0: no more group
 *	1: group exists
 *
 * can be used as follows:
 *
 * void	*handle;
 * cfg_get_group_init(&handle)
 * while (cfg_get_group_next(&handle, &name, &def)) {
 * 	...
 * }
 */
int cfg_get_group_next(void **h,
			str *gname, cfg_def_t **def);

/* Initialize the handle for cfg_diff_next()
 * WARNING: keeps the context lock held, do not forget
 * to release it with cfg_diff_release()
 */
int cfg_diff_init(cfg_ctx_t *ctx,
		void **h);

/* return the pending changes that have not been
 * committed yet
 * can be used as follows:
 *
 * void *handle;
 * if (cfg_diff_init(ctx, &handle)) return -1
 * while (cfg_diff_next(&handle
 *			&group_name, &var_name,
 *			&old_val, &new_val
 *			&val_type)
 * ) {
 *		...
 * }
 * cfg_diff_release(ctx);
 */
int cfg_diff_next(void **h,
			str *gname, str *vname,
			void **old_val, void **new_val,
			unsigned int *val_type);

/* destroy the handle of cfg_diff_next() */
void cfg_diff_release(cfg_ctx_t *ctx);

#endif /* _CFG_CTX_H */
