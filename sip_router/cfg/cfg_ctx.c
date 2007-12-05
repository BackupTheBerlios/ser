/*
 * $Id: cfg_ctx.c,v 1.2 2007/12/05 16:30:57 tirpi Exp $
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

#include <string.h>

#include "cfg_struct.h"
#include "cfg_ctx.h"

/* linked list of all the registered cfg contexts */
static cfg_ctx_t	*cfg_ctx_list = NULL;

/* creates a new config context that is an interface to the
 * cfg variables with write permission
 */
cfg_ctx_t *cfg_register_ctx(cfg_on_declare on_declare_cb)
{
	cfg_ctx_t	*ctx;
	cfg_group_t	*group;
	str		gname;

	/* allocate memory for the new context
	Better to use shm mem, because 'changed' and 'lock'
	must be in shm mem anyway */
	ctx = (cfg_ctx_t *)shm_malloc(sizeof(cfg_ctx_t));
	if (!ctx) {
		LOG(L_ERR, "ERROR: cfg_register_ctx(): not enough shm memory\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(cfg_ctx_t));
	if (lock_init(&ctx->lock) == 0) {
		LOG(L_ERR, "ERROR: cfg_register_ctx(): failed to init lock\n");
		shm_free(ctx);
		return NULL;
	}

	/* add the new ctx to the beginning of the list */
	ctx->next = cfg_ctx_list;
	cfg_ctx_list = ctx;

	/* let the driver know about the already registered groups */
	if (on_declare_cb) {
		ctx->on_declare_cb = on_declare_cb;

		for (	group = cfg_group;
			group;
			group = group->next
		) {
			gname.s = group->name;
			gname.len = group->name_len;
			on_declare_cb(&gname, group->mapping->def);
		}
	}

	return ctx;
}

/* free the memory allocated for the contexts */
void cfg_ctx_destroy(void)
{
	cfg_ctx_t	*ctx, *ctx2;

	for (	ctx = cfg_ctx_list;
		ctx;
		ctx = ctx2
	) {
		ctx2 = ctx->next;
		shm_free(ctx);
	}
	cfg_ctx_list = NULL;
}

/* notify the drivers about the new config definition */
void cfg_notify_drivers(char *group_name, cfg_def_t *def)
{
	cfg_ctx_t	*ctx;
	str		gname;

	gname.s = group_name;
	gname.len = strlen(group_name);

	for (	ctx = cfg_ctx_list;
		ctx;
		ctx = ctx->next
	)
		if (ctx->on_declare_cb)
			ctx->on_declare_cb(&gname, def);
}

/* convert the value to the requested type
 * (only string->str is implemented currently) */
static int convert_val(unsigned int val_type, void *val,
			unsigned int var_type, void **new_val)
{
	static str	s;

	switch (val_type) {
		case CFG_VAR_INT:
			if (CFG_INPUT_MASK(var_type) != CFG_INPUT_INT)
				goto error;
			*new_val = val;
			break;

		case CFG_VAR_STRING:
			if (CFG_INPUT_MASK(var_type) == CFG_INPUT_STR) {
				s.s = val;
				s.len = strlen(s.s);
				*new_val = (void *)&s;
				break;
			}
			if (CFG_INPUT_MASK(var_type) != CFG_INPUT_STRING)
				goto error;
			*new_val = val;
			break;
		default:
			goto error;
	}

	return 0;

error:
	LOG(L_ERR, "ERROR: convert_val(): got a value with type %u, but expected %u\n",
			val_type, CFG_INPUT_MASK(var_type));
	return -1;
}

/* sets the value of a variable without the need of commit
 *
 * return value:
 *   0: success
 *  -1: error
 *   1: variable has not been found
 */
int cfg_set_now(cfg_ctx_t *ctx, str *group_name, str *var_name,
			void *val, unsigned int val_type)
{
	cfg_group_t	*group;
	cfg_mapping_t	*var;
	void		*p, *v;
	cfg_block_t	*block = NULL;
	str		s;
	char		*old_string = NULL;
	char		**replaced = NULL;
	cfg_child_cb_t	*child_cb = NULL;
	int		i;

	/* verify the context even if we do not need it now
	to make sure that a cfg driver has called the function
	(very very weak security) */
	if (!ctx) {
		LOG(L_ERR, "ERROR: cfg_set_now(): context is undefined\n");
		return -1;
	}

	/* look-up the group and the variable */
	if (cfg_lookup_var(group_name, var_name, &group, &var))
		return 1;

	/* check whether we have to convert the type */
	if (convert_val(val_type, val, CFG_INPUT_TYPE(var), &v))
		goto error0;
	
	if (var->def->on_change_cb) {
		/* Call the fixup function.
		There is no need to set a temporary cfg handle,
		becaue a single variable is changed */
		if (var->def->on_change_cb(*(group->handle),
						var_name,
						&v) < 0) {
			LOG(L_ERR, "ERROR: cfg_set_now(): fixup failed\n");
			goto error0;
		}

	} else if ((CFG_VAR_TYPE(var) == CFG_VAR_INT) 
	&& (var->def->min != var->def->max)) {
		/* perform a simple min-max check for integers */
		if (((int)(long)v < var->def->min)
		|| ((int)(long)v > var->def->max)) {
			LOG(L_ERR, "ERROR: cfg_set_now(): integer value is out of range\n");
			goto error0;
		}
	}

	if (cfg_shmized) {
		if (var->def->on_set_child_cb) {
			child_cb = cfg_child_cb_new(var_name,
						var->def->on_set_child_cb);
			if (!child_cb) {
				LOG(L_ERR, "ERROR: cfg_set_now(): not enough shm memory\n");
				goto error0;
			}
		}

		/* make sure that nobody else replaces the global config
		while the new one is prepared */
		CFG_WRITER_LOCK();

		/* clone the memory block, and prepare the modification */
		if (!(block = cfg_clone_global())) goto error;

		p = block->vars+group->offset+var->offset;
	} else {
		/* we are allowed to rewrite the value on-the-fly */
		p = group->vars + var->offset;
	}

	/* set the new value */
	switch (CFG_VAR_TYPE(var)) {
	case CFG_VAR_INT:
		i = (int)(long)v;
		memcpy(p, &i, sizeof(int));
		break;

	case CFG_VAR_STRING:
		/* clone the string to shm mem */
		s.s = v;
		s.len = strlen(v);
		if (!(s.s = cfg_clone_str(s))) goto error;
		memcpy(&old_string, p, sizeof(char *));
		memcpy(p, &s.s, sizeof(char *));
		break;

	case CFG_VAR_STR:
		/* clone the string to shm mem */
		s = *(str *)v;
		if (!(s.s = cfg_clone_str(s))) goto error;
		memcpy(&old_string, p, sizeof(char *));
		memcpy(p, &s, sizeof(str));
		break;

	case CFG_VAR_POINTER:
		memcpy(p, &v, sizeof(void *));
		break;

	}

	if (cfg_shmized) {
		if (old_string) {
			/* prepare the array of the replaced strings,
			they will be freed when the old block is freed */
			replaced = (char **)shm_malloc(sizeof(char *)*2);
			if (!replaced) {
				LOG(L_ERR, "ERROR: cfg_set_now(): not enough shm memory\n");
				goto error;
			}
			replaced[0] = old_string;
			replaced[1] = NULL;
		}
		/* replace the global config with the new one */
		cfg_install_global(block, replaced, child_cb, child_cb);
		CFG_WRITER_UNLOCK();
	} else {
		/* flag the variable because there is no need
		to shmize it again */
		var->flag |= cfg_var_shmized;
	}

	if (val_type == CFG_VAR_INT)
		LOG(L_INFO, "INFO: cfg_set_now(): %.*s.%.*s "
			"has been changed to %d\n",
			group_name->len, group_name->s,
			var_name->len, var_name->s,
			(int)(long)val);
	else
		LOG(L_INFO, "INFO: cfg_set_now(): %.*s.%.*s "
			"has been changed to \"%s\"\n",
			group_name->len, group_name->s,
			var_name->len, var_name->s,
			(char *)val);

	return 0;

error:
	if (cfg_shmized) CFG_WRITER_UNLOCK();
	if (block) cfg_block_free(block);
	if (child_cb) cfg_child_cb_free(child_cb);

error0:
	LOG(L_ERR, "ERROR: cfg_set_now(): failed to set the variable: %.*s.%.*s\n",
			group_name->len, group_name->s,
			var_name->len, var_name->s);


	return -1;
}

/* wrapper function for cfg_set_now */
int cfg_set_now_int(cfg_ctx_t *ctx, str *group_name, str *var_name, int val)
{
	return cfg_set_now(ctx, group_name, var_name, (void *)(long)val, CFG_VAR_INT);
}

/* wrapper function for cfg_set_now */
int cfg_set_now_string(cfg_ctx_t *ctx, str *group_name, str *var_name, char *val)
{
	return cfg_set_now(ctx, group_name, var_name, (void *)val, CFG_VAR_STRING);
}

/* returns the size of the variable */
static int cfg_var_size(cfg_mapping_t *var)
{
	switch (CFG_VAR_TYPE(var)) {

	case CFG_VAR_INT:
		return sizeof(int);

	case CFG_VAR_STRING:
		return sizeof(char *);

	case CFG_VAR_STR:
		return sizeof(str);

	case CFG_VAR_POINTER:
		return sizeof(void *);

	default:
		LOG(L_CRIT, "BUG: cfg_var_sizeK(): unknown type: %u\n",
			CFG_VAR_TYPE(var));
		return 0;
	}
}

/* sets the value of a variable but does not commit the change
 *
 * return value:
 *   0: success
 *  -1: error
 *   1: variable has not been found
 */
int cfg_set_delayed(cfg_ctx_t *ctx, str *group_name, str *var_name,
			void *val, unsigned int val_type)
{
	cfg_group_t	*group;
	cfg_mapping_t	*var;
	void		*v;
	char		*temp_handle;
	int		temp_handle_created;
	cfg_changed_var_t	*changed = NULL;
	int		i, size;
	str		s;

	if (!cfg_shmized)
		/* the cfg has not been shmized yet, there is no
		point in registering the change and committing it later */
		return cfg_set_now(ctx, group_name, var_name,
					val, val_type);

	if (!ctx) {
		LOG(L_ERR, "ERROR: cfg_set_delayed(): context is undefined\n");
		return -1;
	}

	/* look-up the group and the variable */
	if (cfg_lookup_var(group_name, var_name, &group, &var))
		return 1;

	/* check whether we have to convert the type */
	if (convert_val(val_type, val, CFG_INPUT_TYPE(var), &v))
		goto error0;

	/* the ctx must be locked while reading and writing
	the list of changed variables */
	CFG_CTX_LOCK(ctx);

	if (var->def->on_change_cb) {
		/* The fixup function must see also the
		not yet committed values, so a temporary handle
		must be prepared that points to the new config.
		Only the values within the group are applied,
		other modifications are not visible to the callback.
		The local config is the base. */

		if (ctx->changed_first) {
			temp_handle = (char *)pkg_malloc(group->size);
			if (!temp_handle) {
				LOG(L_ERR, "ERROR: cfg_set_delayed(): "
					"not enough memory\n");
				goto error;
			}
			temp_handle_created = 1;
			memcpy(temp_handle, *(group->handle), group->size);

			/* apply the changes */
			for (	changed = ctx->changed_first;
				changed;
				changed = changed->next
			) {
				if (changed->group != group) continue;

				memcpy(	temp_handle + changed->var->offset,
					changed->new_val,
					cfg_var_size(changed->var));
			}
		} else {
			/* there is not any change */
			temp_handle = *(group->handle);
			temp_handle_created = 0;
		}
			
		if (var->def->on_change_cb(temp_handle,
						var_name,
						&v) < 0) {
			LOG(L_ERR, "ERROR: cfg_set_delayed(): fixup failed\n");
			if (temp_handle_created) pkg_free(temp_handle);
			goto error;
		}
		if (temp_handle_created) pkg_free(temp_handle);

	} else if ((CFG_VAR_TYPE(var) == CFG_VAR_INT) 
	&& (var->def->min != var->def->max)) {
		/* perform a simple min-max check for integers */
		if (((int)(long)v < var->def->min)
		|| ((int)(long)v > var->def->max)) {
			LOG(L_ERR, "ERROR: cfg_set_delayed(): integer value is out of range\n");
			goto error;
		}
	}

	/* everything went ok, we can add the new value to the list */
	size = sizeof(cfg_changed_var_t) + cfg_var_size(var) - 1;
	changed = (cfg_changed_var_t *)shm_malloc(size);
	if (!changed) {
		LOG(L_ERR, "ERROR: cfg_set_delayed(): not enough shm memory\n");
		goto error;
	}
	memset(changed, 0, size);
	changed->group = group;
	changed->var = var;

	switch (CFG_VAR_TYPE(var)) {

	case CFG_VAR_INT:
		i = (int)(long)v;
		memcpy(changed->new_val, &i, sizeof(int));
		break;

	case CFG_VAR_STRING:
		/* clone the string to shm mem */
		s.s = v;
		s.len = strlen(v);
		if (!(s.s = cfg_clone_str(s))) goto error;
		memcpy(changed->new_val, &s.s, sizeof(char *));
		break;

	case CFG_VAR_STR:
		/* clone the string to shm mem */
		s = *(str *)v;
		if (!(s.s = cfg_clone_str(s))) goto error;
		memcpy(changed->new_val, &s, sizeof(str));
		break;

	case CFG_VAR_POINTER:
		memcpy(changed->new_val, &v, sizeof(void *));
		break;

	}

	/* Add the new item to the end of the linked list,
	The commit will go though the list from the first item,
	so the list is kept in order */
	if (ctx->changed_first)
		ctx->changed_last->next = changed;
	else
		ctx->changed_first = changed;

	ctx->changed_last = changed;

	CFG_CTX_UNLOCK(ctx);

	if (val_type == CFG_VAR_INT)
		LOG(L_INFO, "INFO: cfg_set_delayed(): %.*s.%.*s "
			"is going to be changed to %d "
			"[context=%p]\n",
			group_name->len, group_name->s,
			var_name->len, var_name->s,
			(int)(long)val,
			ctx);
	else
		LOG(L_INFO, "INFO: cfg_set_delayed(): %.*s.%.*s "
			"is going to be changed to \"%s\" "
			"[context=%p]\n",
			group_name->len, group_name->s,
			var_name->len, var_name->s,
			(char *)val,
			ctx);

	return 0;

error:
	CFG_CTX_UNLOCK(ctx);
	if (changed) shm_free(changed);
error0:
	LOG(L_ERR, "ERROR: cfg_set_delayed(): failed to set the variable: %.*s.%.*s\n",
			group_name->len, group_name->s,
			var_name->len, var_name->s);

	return -1;
}

/* wrapper function for cfg_set_delayed */
int cfg_set_delayed_int(cfg_ctx_t *ctx, str *group_name, str *var_name, int val)
{
	return cfg_set_delayed(ctx, group_name, var_name, (void *)(long)val, CFG_VAR_INT);
}

/* wrapper function for cfg_set_delayed */
int cfg_set_delayed_string(cfg_ctx_t *ctx, str *group_name, str *var_name, char *val)
{
	return cfg_set_delayed(ctx, group_name, var_name, (void *)val, CFG_VAR_STRING);
}

/* commits the previously prepared changes within the context */
int cfg_commit(cfg_ctx_t *ctx)
{
	int	replaced_num = 0;
	cfg_changed_var_t	*changed, *changed2;
	cfg_block_t	*block;
	char	**replaced = NULL;
	cfg_child_cb_t	*child_cb;
	cfg_child_cb_t	*child_cb_first = NULL;
	cfg_child_cb_t	*child_cb_last = NULL;
	int	size;
	void	*p;
	str	s;

	if (!ctx) {
		LOG(L_ERR, "ERROR: cfg_commit(): context is undefined\n");
		return -1;
	}

	if (!cfg_shmized) return 0; /* nothing to do */

	/* the ctx must be locked while reading and writing
	the list of changed variables */
	CFG_CTX_LOCK(ctx);

	/* is there any change? */
	if (!ctx->changed_first) goto done;

	/* count the number of replaced strings,
	and prepare the linked list of per-child process
	callbacks, that will be added to the global list */
	for (	changed = ctx->changed_first;
		changed;
		changed = changed->next
	) {
		if ((CFG_VAR_TYPE(changed->var) == CFG_VAR_STRING)
		|| (CFG_VAR_TYPE(changed->var) == CFG_VAR_STR))
			replaced_num++;


		if (changed->var->def->on_set_child_cb) {
			s.s = changed->var->def->name;
			s.len = changed->var->name_len;
			child_cb = cfg_child_cb_new(&s,
					changed->var->def->on_set_child_cb);
			if (!child_cb) goto error0;

			if (child_cb_last)
				child_cb_last->next = child_cb;
			else
				child_cb_first = child_cb;
			child_cb_last = child_cb;
		}
	}

	/* allocate memory for the replaced string array */
	size = sizeof(char *)*(replaced_num + 1);
	replaced = (char **)shm_malloc(size);
	if (!replaced) {
		LOG(L_ERR, "ERROR: cfg_commit(): not enough shm memory\n");
		goto error;
	}
	memset(replaced, 0 , size);

	/* make sure that nobody else replaces the global config
	while the new one is prepared */
	CFG_WRITER_LOCK();

	/* clone the memory block, and prepare the modification */
	if (!(block = cfg_clone_global())) {
		CFG_WRITER_UNLOCK();
		goto error;
	}

	/* apply the modifications to the buffer */
	replaced_num = 0;
	for (	changed = ctx->changed_first;
		changed;
		changed = changed->next
	) {
		p = block->vars
			+ changed->group->offset
			+ changed->var->offset;

		if ((CFG_VAR_TYPE(changed->var) == CFG_VAR_STRING)
		|| (CFG_VAR_TYPE(changed->var) == CFG_VAR_STR)) {
			memcpy(&(replaced[replaced_num]), p, sizeof(char *));
			replaced_num++;
		}

		memcpy(	p,
			changed->new_val,
			cfg_var_size(changed->var));
	}

	/* replace the global config with the new one */
	cfg_install_global(block, replaced, child_cb_first, child_cb_last);
	CFG_WRITER_UNLOCK();

	/* free the changed list */	
	for (	changed = ctx->changed_first;
		changed;
		changed = changed2
	) {
		changed2 = changed->next;
		shm_free(changed);
	}
	ctx->changed_first = NULL;
	ctx->changed_last = NULL;

done:
	LOG(L_INFO, "INFO: cfg_commit(): config changes have been applied "
			"[context=%p]\n",
			ctx);

	CFG_CTX_UNLOCK(ctx);
	return 0;

error:
	CFG_CTX_UNLOCK(ctx);

error0:

	if (child_cb_first) cfg_child_cb_free(child_cb_first);
	if (replaced) shm_free(replaced);

	return -1;
}

/* drops the not yet committed changes within the context */
int cfg_rollback(cfg_ctx_t *ctx)
{
	cfg_changed_var_t	*changed, *changed2;
	char	*new_string;

	if (!ctx) {
		LOG(L_ERR, "ERROR: cfg_rollback(): context is undefined\n");
		return -1;
	}

	if (!cfg_shmized) return 0; /* nothing to do */

	LOG(L_INFO, "INFO: cfg_rollback(): deleting the config changes "
			"[context=%p]\n",
			ctx);

	/* the ctx must be locked while reading and writing
	the list of changed variables */
	CFG_CTX_LOCK(ctx);

	for (	changed = ctx->changed_first;
		changed;
		changed = changed2
	) {
		changed2 = changed->next;

		if ((CFG_VAR_TYPE(changed->var) == CFG_VAR_STRING)
		|| (CFG_VAR_TYPE(changed->var) == CFG_VAR_STR)) {
			memcpy(&new_string, changed->new_val, sizeof(char *));
			shm_free(new_string);
		}
		shm_free(changed);
	}
	ctx->changed_first = NULL;
	ctx->changed_last = NULL;

	CFG_CTX_UNLOCK(ctx);

	return 0;
}

/* returns the value of a variable */
int cfg_get_by_name(cfg_ctx_t *ctx, str *group_name, str *var_name,
			void **val, unsigned int *val_type)
{
	cfg_group_t	*group;
	cfg_mapping_t	*var;
	void		*p;
	static str	s;	/* we need the value even
				after the function returns */
	int		i;
	char		*ch;

	/* verify the context even if we do not need it now
	to make sure that a cfg driver has called the function
	(very very weak security) */
	if (!ctx) {
		LOG(L_ERR, "ERROR: cfg_get_by_name(): context is undefined\n");
		return -1;
	}

	/* look-up the group and the variable */
	if (cfg_lookup_var(group_name, var_name, &group, &var))
		return -1;

	/* use the module's handle to access the variable
	It means that the variable is read from the local config
	after forking */
	p = *(group->handle) + var->offset;

	switch (CFG_VAR_TYPE(var)) {
	case CFG_VAR_INT:
		memcpy(&i, p, sizeof(int));
		*val = (void *)(long)i;
		break;

	case CFG_VAR_STRING:
		memcpy(&ch, p, sizeof(char *));
		*val = (void *)ch;
		break;

	case CFG_VAR_STR:
		memcpy(&s, p, sizeof(str));
		*val = (void *)&s;
		break;

	case CFG_VAR_POINTER:
		memcpy(val, &p, sizeof(void *));
		break;

	}
	*val_type = CFG_VAR_TYPE(var);

	return 0;
}

/* returns the description of a variable */
int cfg_help(cfg_ctx_t *ctx, str *group_name, str *var_name,
			char **ch)
{
	cfg_mapping_t	*var;

	/* verify the context even if we do not need it now
	to make sure that a cfg driver has called the function
	(very very weak security) */
	if (!ctx) {
		LOG(L_ERR, "ERROR: cfg_help(): context is undefined\n");
		return -1;
	}

	/* look-up the group and the variable */
	if (cfg_lookup_var(group_name, var_name, NULL, &var))
		return -1;

	*ch = var->def->descr;
	return 0;
}
