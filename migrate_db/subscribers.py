#!/usr/bin/python2.3
#
# $Id: subscribers.py,v 1.1 2006/11/17 00:20:00 janakj Exp $
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

import common, MySQLdb
from uid import generate_uid

def convert_subscribers(table, dst, src, flags):
    cursor = src.cursor()
    if common.verbose: print "Converting %s table into credentials and user_attrs tables" % table

    try:
        cursor.execute("select username, password, first_name, last_name, email_address, datetime_created,"
                       "ha1, domain, ha1b, timezone, rpid from %s" % table);
    except MySQLdb.ProgrammingError:
        print "Error while querying %s table, skipping" % table
        return
    
    if cursor.rowcount == 0:
        if common.verbose:
            print "No rows in %s table, nothing to convert" % table
        return

    user = cursor.fetchone()
    while user:
        uid = generate_uid()
        try:
            dst.cursor().execute("insert into credentials (auth_username, realm, password, flags, ha1, ha1b, uid)"
                                 "values (%s, %s, %s, %s, %s, %s, %s)",
                                 (user[0], user[7], user[1], common.DB_LOAD_SER | common.DB_FOR_SERWEB | flags, user[6], user[8], uid))
        except MySQLdb.IntegrityError:
            print "Conflicting row found in target credentials table"
            print "Make sure the table is empty and re-run the script"
            sys.exit(1)

        try:
            dst.cursor().execute("insert into user_attrs (uid, name, value, flags) values (%s, %s, %s, %s)",
                                 (uid, 'datetime_created', user[5], common.DB_FOR_SERWEB | flags))
            if user[2]:
                dst.cursor().execute("insert into user_attrs (uid, name, value, flags) values (%s, %s, %s, %s)",
                                     (uid, 'first_name', user[2], common.DB_FOR_SERWEB | flags))
            if user[3]:
                dst.cursor().execute("insert into user_attrs (uid, name, value, flags) values (%s, %s, %s, %s)",
                                     (uid, 'last_name', user[3], common.DB_FOR_SERWEB | flags))
            if user[4]:
                dst.cursor().execute("insert into user_attrs (uid, name, value, flags) values (%s, %s, %s, %s)",
                                             (uid, 'email', user[4], common.DB_LOAD_SER | common.DB_FOR_SERWEB | flags))
            if user[9]:
                dst.cursor().execute("insert into user_attrs (uid, name, value, flags) values (%s, %s, %s, %s)",
                                     (uid, 'timezone', user[9], common.DB_FOR_SERWEB | flags))
            if user[10]:
                dst.cursor().execute("insert into user_attrs (uid, name, value, flags) values (%s, %s, %s, %s)",
                                     (uid, 'asserted_id', user[10], common.DB_LOAD_SER | common.DB_FOR_SERWEB | flags))
        except MySQLdb.IntegrityError:
            print "Conflicting row found in target user_attrs table"
            print "Make sure the table is empty and re-run the script"
            sys.exit(1)

        user = cursor.fetchone()
