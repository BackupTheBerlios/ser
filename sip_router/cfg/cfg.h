/*
 * $Id: cfg.h,v 1.3 2008/01/28 12:23:11 tirpi Exp $
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

#ifndef _CFG_H
#define _CFG_H

#include "../str.h"

#define CFG_VAR_INT		1U
#define CFG_VAR_STRING		2U
#define CFG_VAR_STR		3U
#define CFG_VAR_POINTER		4U

#define CFG_INPUT_SHIFT		3

#define CFG_INPUT_INT		(CFG_VAR_INT << CFG_INPUT_SHIFT)
#define CFG_INPUT_STRING	(CFG_VAR_STRING << CFG_INPUT_SHIFT)
#define CFG_INPUT_STR		(CFG_VAR_STR << CFG_INPUT_SHIFT)

#define CFG_VAR_MASK(x)		((x)&(CFG_INPUT_INT-1))
#define CFG_INPUT_MASK(x)	((x)&(~(CFG_INPUT_INT-1)))

typedef int (*cfg_on_change)(void *, str *, void **);
typedef void (*cfg_on_set_child)(str *);

/* strutrure to be used by the module interface */
typedef struct _cfg_def {
	char	*name;
	unsigned int	type;
	int	min;
	int	max;
	cfg_on_change	on_change_cb;
	cfg_on_set_child	on_set_child_cb;
	char	*descr;
} cfg_def_t;

/* declares a new cfg group
 * handler is set to the memory area where the variables are stored
 * return value is -1 on error
 */
int cfg_declare(char *group_name, cfg_def_t *def, void *values, int def_size,
			void **handler);

#define cfg_size(gname) \
	sizeof(struct cfg_group_##gname)

#define cfg_get(gname, handle, var) \
	((struct cfg_group_##gname *)handle)->var

/* declares a single variable with integer type */
int cfg_declare_int(char *group_name, char *var_name, int val, char *descr);

/* declares a single variable with str type */
int cfg_declare_str(char *group_name, char *var_name, char *val, char *descr);

#endif /* _CFG_H */
