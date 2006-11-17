#!/usr/bin/python2.3
#
# $Id: domain.py,v 1.1 2006/11/17 00:17:27 janakj Exp $
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

import sys
import MySQLdb
import common
from did import generate_did

class Domain:
    def __init__(self, domain):
        self.did = generate_did()   # Domain ID
        self.domain = domain  # Domain name
        self.aliases = []     # List of aliases
        if common.verbose: print "Created virtual domain %s" % self.domain

    def match(self, name):
        if name.endswith('.' + self.domain):
            return True
        else:
            return False

    def add(self, alias):
        self.aliases.append(alias)
    
    def write(self, db):
        db.cursor().execute("insert into domain (did, domain, flags) values (%s, %s, %s)",
                            (self.did, self.domain, common.DB_LOAD_SER | common.DB_FOR_SERWEB));
        db.cursor().execute("insert into domain_attrs (did, name, value, flags) values (%s, %s, %s, %s)",
                            (self.did, "digest_realm", self.domain, common.DB_LOAD_SER | common.DB_CANON | common.DB_FOR_SERWEB))
        for alias in self.aliases:
            db.cursor().execute("insert into domain (did, domain, flags) values (%s, %s, %s)",
                                (self.did, alias, common.DB_LOAD_SER | common.DB_FOR_SERWEB));

def domain_name_cmp(x, y):
    return len(x.split('.')) - len(y.split('.'))

def convert_domains(dst, src):
    global verbose
    domains = []
    cursor = src.cursor()

    if common.verbose:
        print "Converting domain table"

    try:
        cursor.execute("select domain from domain");
    except MySQLdb.ProgrammingError:
        print "Error while querying domain table, skipping"
        return
    
    if cursor.rowcount == 0:
        if common.verbose:
            print "No rows in source domain table, nothing to convert"
        return

    l = []
    for i in cursor.fetchall():
        l.append(i[0])
    l.sort(domain_name_cmp)

    for i in l:
        found = False
        for d in domains:
            if d.match(i):
                d.add(i)
                found = True
        if not found: domains.append(Domain(i))

    for d in domains:
        try:
            d.write(dst)
        except MySQLdb.IntegrityError:
            print "Conflicting row found in target domain table"
            print "Make sure that target domain table is empty and re-run the script"
            sys.exit(1)
