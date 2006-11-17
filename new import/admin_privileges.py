#!/usr/bin/python2.3
#
# $Id: admin_privileges.py,v 1.1 2006/11/17 00:17:27 janakj Exp $
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

def convert_admin_privileges(dst, src):
    cur = src.cursor();

    if common.verbose: print "Converting admin_privileges table"

    try:
        cur.execute("select username, domain, priv_name, priv_value from admin_privileges")
    except MySQLdb.ProgrammingError:
        print "Error while querying admin_privileges table, skipping"
        return

    privs = cur.fetchall()

    if common.verbose and cur.rowcount == 0:
        print "Source admin_privileges table is empty, skipping"
        return

    for priv in privs:
        (username, domain, priv_name, priv_value) = priv
        domain = common.extract_domain(username, domain)

        try:
            uid = get_uid_by_uri(dst, username, domain)
            if priv_name == "is_admin":
                dst.cursor().execute("insert into user_attrs (uid, name, type, value, flags) values "
                                     "(%s, %s, %s, %s, %s)", (uid, "sw_is_admin", 0, 1, common.DB_FOR_SERWEB))
            elif priv_name == "acl_control":
                dst.cursor().execute("insert into user_attrs (uid, name, value, type, flags) values "
                                     "(%s, %s, %s, %s, %s)", (uid, "sw_acl_control", priv_value, 2, common.DB_FOR_SERWEB))
            elif priv_name == "change_privileges":
                dst.cursor().execute("insert into user_attrs (uid, name, type, value, flags) values "
                                     "(%s, %s, %s, %s, %s)", (uid, "sw_is_hostmaster", 0, 1, common.DB_FOR_SERWEB))
            else:
                print "ERROR: Unsupported privilege '%s' in admin_privileges table, skipping" % priv_name
        except UidNotFoundException:
            print "ERROR: Cannot find UID for admin_privilege entry for '%s@%s'" % (username, domain)

        except MySQLdb.IntegrityError:
            print "Conflicting row found user_attrs table"
            sys.exit(1)
