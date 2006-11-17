#!/usr/bin/python2.3
#
# $Id: msilo.py,v 1.1 2006/11/17 00:17:27 janakj Exp $
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
from did import get_did_by_domain, DidNotFoundException
from time import *

def convert_msilo(dst, src):

    if common.verbose: print "Converting msilo table"
    
    cur = src.cursor();

    try:
        cur.execute("select username, domain, src_addr, dst_addr, r_uri, inc_time, exp_time, ctype, body from silo")
    except MySQLdb.ProgrammingError:
        print "Error while querying msilo table, skipping"
        return
    
    msgs = cur.fetchall()

    if common.verbose and cur.rowcount == 0:
        print "Source msilo table is empty, skipping"
        return

    for msg in msgs:
        domain = extract_domain(msg[0], msg[1])
        inc_time = strftime("%Y-%m-%d %H:%M:%S", gmtime(msg[5]))
        exp_time = strftime("%Y-%m-%d %H:%M:%S", gmtime(msg[6]))
        try:
            uid = get_uid_by_uri(dst, msg[0], domain)
            dst.cursor().execute("insert into silo (uid, ruri, from_hdr, to_hdr, inc_time, exp_time, ctype, body) "
                                 "values (%s, %s, %s, %s, %s, %s, %s, %s)",
                                 (uid, msg[4], msg[2], msg[3], inc_time, exp_time, msg[7], msg[8]))
            
        except UidNotFoundException:
            print "ERROR: Cannot find UID for stored message for '%s@%s'" % (msg[0], domain)

        except MySQLdb.IntegrityError:
            print "Conflicting row found in target msilo table"
            print "Make sure that the target msilo table is empty and re-run the script"
            sys.exit(1)

