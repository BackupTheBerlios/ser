#!/usr/bin/python2.3
#
# $Id: phonebook.py,v 1.1 2006/11/17 00:20:00 janakj Exp $
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

def convert_phonebook(dst, src):
    if common.verbose: print "Converting phonebook table"
    
    cur = src.cursor();

    try:
        cur.execute("select username, domain, fname, lname, sip_uri from phonebook")
    except MySQLdb.ProgrammingError:
        print "Error while querying phonebook table, skipping"
        return
        
    pbs = cur.fetchall()

    if common.verbose and cur.rowcount == 0:
        print "Source phonebook table is empty, skipping"
        return

    for pb in pbs:
        domain = extract_domain(pb[0], pb[1])
        try:
            uid = get_uid_by_uri(dst, pb[0], domain)

            dst.cursor().execute("insert into phonebook (uid, fname, lname, sip_uri) "
                                 "values (%s, %s, %s, %s)",
                                 (uid, pb[2], pb[3], pb[4]))
                
        except UidNotFoundException:
            print "ERROR: Cannot find UID for phonebook entry for '%s@%s'" % (pb[0], domain)

        except MySQLdb.IntegrityError:
            print "Conflicting row found in target phonebook table"
            print "Make sure that the target phonebook table is empty and re-run the script"
            sys.exit(1)

