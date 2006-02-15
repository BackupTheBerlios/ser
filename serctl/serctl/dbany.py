#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: dbany.py,v 1.5 2006/02/15 18:51:29 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.dbmysql    import DBmysql
from serctl.dbpostgres import DBpostgres
from serctl.dbbase     import DSC_NAME, DSC_TYPE, DSC_DEFAULT
from serctl.error      import error, EINVAL
from serctl.uri        import parse, SCHEME

DBS = {\
	'mysql'    : DBmysql,
	'postgres' : DBpostgres,
}

def DBany(uri):
	puri = parse(uri)
	if not DBS.has_key(puri[SCHEME]):
		error(EINVAL, uri)
	db = DBS[puri[SCHEME]]
	return db(puri)
