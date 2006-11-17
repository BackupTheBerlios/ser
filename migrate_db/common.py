#!/usr/bin/python2.3
#
# $Id: common.py,v 1.1 2006/11/17 00:19:59 janakj Exp $
#
# Copyright (C) 2006 iptelorg GmbH
#
# This script is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version
#
# This script is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

DEFAULT_DOMAIN = "_none"
DEFAULT_DID = "0"
DEFAULT_UID = "0"

DB_LOAD_SER   = 1 << 0
DB_DISABLED   = 1 << 1
DB_CANON      = 1 << 2
DB_IS_TO      = 1 << 3
DB_IS_FROM    = 1 << 4
DB_FOR_SERWEB = 1 << 5
DB_DELETED    = 1 << 7
DB_CALLER_DELETED = 1 << 8
DB_CALLEE_DELETED = 1 << 9

verbose = False

#
# Try to extract domain part from username if domain
# parameter is empty or NULL, return DEFAULT_DOMAIN
# if everything fails
#
def extract_domain(username, domain = None):
    if domain is None or domain == "":
        l = username.split('@')
        if len(l) == 2:
            return l[1]
        else:
            return DEFAULT_DOMAIN
    else:
        return domain
