#!/usr/bin/python2.3
#
# $Id: did.py,v 1.1 2006/11/17 00:19:59 janakj Exp $
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

from common import *
import uuid

class DidNotFoundException(Exception):
    def __init__(self, domain):
        self.domain = domain
    def __str__(self):
        return repr(self.domain)


def get_did_by_domain(db, domain):
    if domain is None:
        raise DidNotFoundException(domain)
    if domain == DEFAULT_DOMAIN:
        return DEFAULT_DID
    cur = db.cursor();
    cur.execute("select did from domain where domain=%s", (domain))
    try:
        return cur.fetchone()[0]
    except:
        raise DidNotFoundException(domain)


def generate_did():
    return str(uuid.uuid1())[1:-1]
