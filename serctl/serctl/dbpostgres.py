#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: dbpostgres.py,v 1.4 2006/01/12 17:07:18 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from dbsql import DBsql

class DBpostgres(DBsql):

	def _connect(self, user, password, host, port, db):
		import psycopg

		params  = []
		if user is not None:
			params.append("user='%s'" % user)
		if password is not None:
			params.append("password='%s'" % password)
		if host is not None:
			params.append("host='%s'" % host)
		if port is not None:
			params.append("port='%s'" % port)
		if db is not None:
			params.append("database='%s'" % db)
		params = ', '.join(params)

		connect = 'psycopg.connect(%s)' % params
		return eval(connect)

	def describe(self, tab):
		db = self.db.cursor()
		sql = "SELECT column_name, data_type, column_default " \
		      "FROM information_schema.columns"
		where = self._where(('=', 'table_name', tab))
		db.execute(sql + where)
                descs = {}
                for row in db.fetchall():
			name    = row[0]
			type    = '?'   # FIX: not-implemented
			default = '?'   # FIX: not-implemented
			descs[name] = (name, type, default, )
                db.close()
                return descs

