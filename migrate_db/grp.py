#!/usr/bin/python2.3
#
# $Id: grp.py,v 1.1 2006/11/17 00:19:59 janakj Exp $
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

import common, MySQLdb, sys

from uid import get_uid_by_uri, UidNotFoundException

def convert_grp(dst, src):

    if common.verbose: print "Converting grp table into user attributes"
    
    cur = src.cursor();

    try:
        cur.execute("select username, domain, grp from grp")
        groups = cur.fetchall()
    except MySQLdb.ProgrammingError:
        print "Error while querying grp table, skipping"
        return

    for group in groups:
        (username, domain, grp) = group
        domain = extract_domain(username, domain)
        try:
            uid = get_uid_by_uri(dst, username, domain)
            dst.cursor().execute("insert into user_attrs (uid, name, type, value, flags) values"
                                 "(%s, %s, %s, %s, %s)", (uid, "acl", 2, grp, common.DB_LOAD_SER | common.DB_FOR_SERWEB))
        except UidNotFoundException:
            print "ERROR: Cannot find UID for group entry for '%s@%s'" % (username, domain)

        except MySQLdb.IntegrityError:
            print "Conflicting row found in user_attrs table"
            sys.exit(1)
