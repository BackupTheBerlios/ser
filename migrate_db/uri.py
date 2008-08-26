#!/usr/bin/python2.3
#
# $Id: uri.py,v 1.2 2008/08/26 07:40:30 janakj Exp $
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

import common,MySQLdb,sys

from uid import get_uid_by_uri, UidNotFoundException
from did import get_did_by_domain, DidNotFoundException
from parse_uri import *
from common import *


#
# Generate the contents of URI table
# from credentials usernames
#
def convert_uri_from_credentials(dst, src):
    cur = dst.cursor()

    if common.verbose: print "Converting usernames and realms from credentials table into URIs in uri table"

    try:
        cur.execute("select uid, auth_username, realm from credentials")
        
    except MySQLdb.ProgrammingError:
        print "Error while querying credentials table, skipping"
        return

    if cur.rowcount == 0:
        if common.verbose:
            print "credentials table is empty, nothing to convert"
        return
    
    users = cur.fetchall()

    for u in users:
        try:
            did = get_did_by_domain(dst, u[2])
            dst.cursor().execute("insert into uri (uid, did, username, flags) values"
                                 "(%s, %s, %s, %s)", (u[0], did, u[1],
                                                      common.DB_LOAD_SER | common.DB_IS_FROM | common.DB_IS_TO | common.DB_CANON | common.DB_FOR_SERWEB))
        except DidNotFoundException:
            dst.cursor().execute("insert into uri (uid, did, username, flags) values"
                                 "(%s, %s, %s, %s)", (u[0], common.DEFAULT_DOMAIN, u[1],
                                                      common.DB_LOAD_SER | common.DB_IS_FROM | common.DB_IS_TO | common.DB_CANON | common.DB_FOR_SERWEB))
        except MySQLdb.IntegrityError:
            print "Confliting row found in target uri table"
            print "Make sure that the target database is empty and rerun the script"
            sys.exit(1)



def convert_uri_from_aliases(dst, src):
    cur = src.cursor()

    if common.verbose: print "Converting usernames and domains and contacts from aliases table into URIs in uri table"

    try:
        cur.execute("select username, domain, contact from aliases")
    except MySQLdb.ProgrammingError:
        print "Error while querying aliases table, skipping"
        return

    if cur.rowcount == 0:
        if common.verbose:
            print "Aliases table is empty, nothing to convert"
        return
    
    aliases = cur.fetchall()

    for alias in aliases:
        domain = extract_domain(alias[0], alias[1])
        (scheme, username, password, host, port, params) = parse(alias[2]);
        if scheme is None:
            print "Malformed URI found in aliases table: '%s', skipping" % alias[2]
            continue
        
        try:
            did = get_did_by_domain(dst, domain)
        except DidNotFoundException:
            did = common.DEFAULT_DID
            
        try:
            uid = get_uid_by_uri(dst, username, host)
            dst.cursor().execute("insert into uri (uid, did, username, flags) values"
                                 "(%s, %s, %s, %s)", (uid, did, alias[0],
                                                      common.DB_LOAD_SER | common.DB_IS_FROM | common.DB_IS_TO | common.DB_FOR_SERWEB))
        except UidNotFoundException:
            print "ERROR: Could not find UID for alias target '%s@%s', skipping" % (username, host)
            
        except MySQdb.IntegrityError:
            print "Conflicting row found in uri table"
            print "Make sure that the table is empty and re-run the script"
            sys.exit(1)

                
def convert_uri_from_uri(dst, src):
    cur = src.cursor()

    if common.verbose: print "Converting source uri table into destination uri table"

    try:
        cur.execute("select username, domain, uri_user from uri")
    except MySQLdb.ProgrammingError:
        print "Error while querying source uri table, skipping"
        return
    
    uris = cur.fetchall()
    if cur.rowcount == 0:
        if common.verbose: print "Empty source uri table, nothing to convert"
        return

    for uri in uris:
        domain = extract_domain(uri[0], uri[1])
        try:
            did = get_did_by_domain(dst, domain)
        except DidNotFoundException:
            did = common.DEFAULT_DID

        try:
            uid = get_uid_by_uri(dst, uri[0], domain, did)
            dst.cursor().execute("insert into uri (uid, did, username, flags) values"
                                 "(%s, %s, %s, %s)", (uid, did, uri[2],
                                                      common.DB_LOAD_SER | common.DB_IS_FROM | common.DB_IS_TO | common.DB_FOR_SERWEB))
        except UidNotFoundException:
            print "ERROR: Could not find UID for '%s@%s' from uri table" % (uri[0], domain)

        except MySQLdb.IntegrityError:
            print "Conflicting row found in uri table"
            print "Make sure that the table is empty and re-run the script"
            sys.exit(1)
