#!/usr/bin/python2.3
#
# $Id: acc.py,v 1.1 2006/11/17 00:19:59 janakj Exp $
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

from common import *

from uid import get_uid_by_uri, UidNotFoundException
from did import get_did_by_domain, DidNotFoundException
from parse_uri import parse

def convert_acc(table, dst, src):

    if common.verbose: print "Converting %s table" % table

    start = 0
    count = 1000
    num = 1

    while True:
        cur = src.cursor()
        try:
            cur.execute("select sip_from, sip_to, sip_status, sip_method, i_uri, o_uri, "
                        "from_uri, to_uri, sip_callid, username, fromtag, totag, time, "
                        "timestamp, domain from %s limit %s, %s" % (table, start, count))
        except MySQLdb.ProgrammingError:
            print "Error while querying %s table, skipping" % table
            
        if cur.rowcount == 0:
            return
            
        acc = cur.fetchone()
        while acc:
            cols = []
            vals = []

            # time
            cols.append("request_timestamp")
            vals.append(acc[13])
            
            cols.append("response_timestamp")
            vals.append(acc[13])
            
            # sip_from
            cols.append("sip_from")
            vals.append(acc[0])
            
            # from_uri
            did = None
            cols.append("from_uri")
            vals.append(acc[6])
            # Parse the URI in from_uri column
            (scheme, user, passwd, domain, port, params) = parse(acc[6])
            # Find out did
            try:
                did = get_did_by_domain(dst, domain)
                cols.append("from_did")
                vals.append(did)
            except DidNotFoundException:
                pass

            # Try to find out UID if we have did
            if did is not None and user is not None:
                try:
                    uid = get_uid_by_uri(dst, user, domain, did)
                    cols.append("from_uid")
                    vals.append(uid)
                except UidNotFoundException:
                    pass
                
            # sip_to
            cols.append("sip_to")
            vals.append(acc[1])
            
            # sip_status
            cols.append("sip_status")
            vals.append(acc[2])
            
            # sip_method
            cols.append("sip_method")
            vals.append(acc[3])
            
            # i_uri
            did = None
            cols.append("in_ruri")
            vals.append(acc[4])
            # Parse the URI in in_ruri column
            (scheme, user, passwd, domain, port, params) = parse(acc[4])
            # Try to find DID
            try:
                did = get_did_by_domain(dst, domain)
            except DidNotFoundException:
                # We could not find DID, let's try to_uri instead of i_uri
                (scheme, user, passwd, domain, port, params) = parse(acc[7])
                try:
                    did = get_did_by_domain(dst, domain)
                except DidNotFoundException:
                    pass

            # If we have DID try to find UID
            if did is not None:
                cols.append("to_did")
                vals.append(did)

                if user is not None:
                    try:
                        uid = get_uid_by_uri(dst, user, domain, did)
                        cols.append("to_uid")
                        vals.append(uid)
                    except UidNotFoundException:
                        pass
                
            # o_uri
            cols.append("out_ruri")
            vals.append(acc[5])
            
            # to_uri
            cols.append("to_uri")
            vals.append(acc[7])
            
            # sip_callid
            cols.append("sip_callid")
            vals.append(acc[8])
            
            # username
            cols.append("digest_username")
            vals.append(acc[9])
            
            # fromtag
            cols.append("from_tag")
            vals.append(acc[10])
            
            # totag
            cols.append("to_tag")
            vals.append(acc[11])        
            
            # domain
            cols.append("digest_realm")
            vals.append(acc[14])

            query = "insert into %s (" % table
            query += cols[0]
            for c in cols[1:]:
                query += ", "
                query += c
            query += ") values ("
            query += "%s"
            for c in cols[1:]:
                query += ", %s"
            query += ")"

            try:
                dst.cursor().execute(query, tuple(vals))
            except:
                print "Error while executing:"
                print vals
                sys.exit(1)

            num = num + 1
            acc = cur.fetchone()

        start += count
