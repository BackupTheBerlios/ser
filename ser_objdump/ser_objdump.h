/*
 * $Id: ser_objdump.h,v 1.1 2007/02/08 17:33:18 janakj Exp $
 *
 * Copyright (C) 2007 iptel.org
 *
 * ser_objdump is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * ser_objdump is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _SER_OBJDUMP_H

enum export_type {
    TYPE_FUNC,
    TYPE_RPC,
    TYPE_PARAM,
	TYPE_SELECT
};

enum {
    FMT_CSV,
    FMT_SQL
};

struct res {
    char           *module;
    char           *version;
    char           *name;
    enum export_type type;
    int             params;
    unsigned int    flags;
    char           *doc;
};

extern struct res res;

void print_res(struct res *res);

unsigned long relocate(void *addr);

#endif /* _SER_OBJDUMP_H */
