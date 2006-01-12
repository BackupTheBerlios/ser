#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: dbany.py,v 1.4 2006/01/12 14:00:47 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from dbmysql    import DBmysql
from dbpostgres import DBpostgres
from dbbase     import DSC_NAME, DSC_TYPE, DSC_DEFAULT
from error      import error, EINVAL
from uri        import parse, SCHEME

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
