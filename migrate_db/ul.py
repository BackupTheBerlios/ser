#!/usr/bin/python2.3
#
# $Id: ul.py,v 1.1 2006/11/17 00:19:59 janakj Exp $
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

import MySQLdb, sys, common

from uid import get_uid_by_uri, UidNotFoundException

def convert_location(dst, src):

    if common.verbose: print "Converting user location table entries"

    cur = src.cursor();

    try:
        cur.execute("select username, domain, contact, expires, q, callid, "
                    "cseq, flags, received, user_agent from location")
        contacts = cur.fetchall()
    except MySQLdb.ProgrammingError:
        print "Error while querying source location table, skipping"
        return

    if cur.rowcount == 0 and common.verbose:
        print "Source location table is empty, nothing to convert"
        return

    for contact in contacts:
        domain = extract_domain(contact[0], contact[1])
        try:
            uid = get_uid_by_uri(dst, contact[0], domain)
            dst.cursor().execute("insert into location (uid, contact, expires, q, callid, cseq, flags, received, user_agent) "
                                 "values (%s, %s, %s, %s, %s, %s, %s, %s, %s)",
                                 (uid, contact[2], contact[3], contact[4], contact[5], contact[6],
                                  contact[7], contact[8], contact[9]))
        except UidNotFoundException:
            print "ERROR: Could not find UID for location entry '%s@%s', skipping" % (contact[0], domain)

                
def convert_aliases(dst, src):
    # Not implemented
    pass

def convert_ul(dst, src):
    convert_location(dst, src)
    convert_aliases(dst, src)
