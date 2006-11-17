#!/usr/bin/python2.3
#
# $Id: uid.py,v 1.1 2006/11/17 00:20:00 janakj Exp $
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

from did import get_did_by_domain, DidNotFoundException
import uuid, common

class UidNotFoundException(Exception):
    def __init__(self, username, domain):
        self.username = username
        self.domain = domain
    def __str__(self):
        return repr(self.username + "@" + self.domain)

#
# Find UID from username and did/domain pair
#
def get_uid_by_uri(db, username, domain, did = None):
    # No username or domain given, that's bad, raise an exception
    if username is None or domain is None:
        raise UidNotFoundException(username, domain)

    if username.startswith("sip:"): username = username[4:]
    if username.startswith("sips:"): username = username[5:]

    if did is None:
        try:
            did = get_did_by_domain(db, domain)
        except DidNotFoundException:
            did = common.DEFAULT_DID

    cur = db.cursor();
    cur.execute("select uid from uri where username=%s and did=%s", (username, did))
    try:
        return cur.fetchone()[0]
    except:
        # Entry not found in URI table, try digest credentials
        return get_uid_by_credentials(db, username, domain)

#
# Find UID from digest credentials
#
def get_uid_by_credentials(db, username, realm):
    if username is None or realm is None:
        raise UidNotFoundException(username, realm)

    if username.startswith("sip:"): username = username[4:]
    if username.startswith("sips:"): username = username[5:]
    
    cur = db.cursor();
    cur.execute("select uid from credentials where auth_username=%s and realm=%s", (username, realm))
    try:
        return cur.fetchone()[0]
    except:
        raise UidNotFoundException(username, realm)


def generate_uid():
    return str(uuid.uuid1())[1:-1]
