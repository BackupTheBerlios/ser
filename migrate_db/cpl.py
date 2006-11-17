#!/usr/bin/python2.3
#
# $Id: cpl.py,v 1.1 2006/11/17 00:20:00 janakj Exp $
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

def convert_cpl(dst, src):
    cur = src.cursor();

    if common.verbose: print "Converting cpl table"

    try:
        cur.execute("select user, cpl_xml, cpl_bin from cpl")
        cpls = cur.fetchall()
    except MySQLdb.ProgrammingError:
        print "Error while querying cpl table, skipping"
        return

    for cpl in cpls:
        domain = extract_domain(cpl[0])
        try:
            uid = get_uid_by_uri(dst, cpl[0], domain)
            try:
                dst.cursor().execute("insert into cpl (uid, cpl_xml, cpl_bin) values (%s, %s, %s)",
                                     (uid, cpl[1], cpl[2]))
            except MySQLdb.IntegrityViolation:
                print "Conflicting row found in target cpl table"
                print "Make sure that the target table is empty and re-run the script"
                sys.exit(1)
                
        except UidNotFoundException:
            print "ERROR: Could not find UID for cpl table user '%s'" % (cpl[0])

