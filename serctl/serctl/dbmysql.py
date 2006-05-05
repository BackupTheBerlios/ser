#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: dbmysql.py,v 1.5 2006/05/05 11:27:19 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.dbsql import DBsql

class DBmysql(DBsql):

	def _connect(self, user, password, host, port, db):
		import MySQLdb

		params  = []
		if user is not None:
			params.append("user='%s'" % user)
		if password is not None:
			params.append("passwd='%s'" % password)
		if host is not None:
			params.append("host='%s'" % host)
		if port is not None:
			params.append("port=%s" % port)
		if db is not None:
			params.append("db='%s'" % db)
		params = ', '.join(params)

		connect = 'MySQLdb.connect(%s)' % params
		return eval(connect)
