/*
 * $Id: shm_regex.h,v 1.1 2009/04/03 13:03:46 tirpi Exp $
 *
 * Copyright (C) 2009 iptelorg GmbH
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
 *  2009-04-03	Initial version (Miklos)
 */

#ifndef _SHM_REGEX_H
#define _SHM_REGEX_H

#include <sys/types.h>
#include <regex.h>

int shm_regcomp(regex_t *preg, const char *regex, int cflags);
void shm_regfree(regex_t *preg);
int shm_regexec(const regex_t *preg, const char *string, size_t nmatch,
                   regmatch_t pmatch[], int eflags);

#define shm_regerror	regerror

#endif /* _SHM_REGEX_H */
