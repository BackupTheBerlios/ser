#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: dbsql.py,v 1.1 2005/12/21 18:18:30 janakj Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# Created:     2005/11/30
# Last update: 2005/12/17
#
# Base class for SQL databases.

from dbbase import DBbase
from opsql  import where
from uri    import USER, PASS, HOST, PORT, DB

class DBsql(DBbase):
#	dbmodule = <database module>

	def __init__(self, puri):
		params  = []
		if puri[USER] is not None:
			params.append("user='%s'" % puri[USER])
		if puri[PASS] is not None:
			params.append("passwd='%s'" % puri[PASS])
		if puri[HOST] is not None:
			params.append("host='%s'" % puri[HOST])
		if puri[PORT] is not None:
			params.append("port='%s'" % puri[PORT])
		if puri[DB] is not None:
			params.append("db='%s'" % puri[DB])
		params = ', '.join(params)
		connect = 'self.dbmodule.connect(%s)' % params
		self.db = eval(connect)

	def insert(self, tab, ins):
		names  = ins.keys()
		if not names:
			return
		db = self.db.cursor()
		values = [ str(ins[k]) for k in names ]
		names = ','.join(names)
		ss = ['%s'] * len(values)
		ss = ', '.join(ss)
		sql = 'INSERT INTO ' + tab + ' (' + names + ') VALUES (' + ss + ')'
		db.execute(sql, values)
		db.close()

	def _where(self, cond):
		if cond is None:
			return ''
		s = where(cond)
		if s:
			return ' WHERE ' + s
		return ''

	def describe(self, tab):
		db = self.db.cursor()
		db.execute('DESCRIBE ' + tab)
		descs = {}
		for row in db.fetchall():
			name = row[0]
			type = '?'	# FIX: not-implemented
			default = row[4]
			descs[name] = (name, type, default, )
		db.close()
		return descs

	def select(self, tab, cols, conds, limit=0):
		db = self.db.cursor()
		if cols is None:
			cols = '*'
		if type(cols) is str:
			cols = (cols,)
		if not cols:
			return []
		cols = ', '.join(cols)
		sql = 'SELECT %s FROM %s' % (cols, tab) + self._where(conds)
		if limit:
			sql += ' LIMIT ' + str(limit)
		db.execute(sql)
		rows = []
		for row in db.fetchall():
			rows.append([ str(i) for i in row ])
		db.close()
		return rows

	def update(self, tab, ins, conds, limit=0):
		names  = ins.keys()
		if not names:
			return
		db = self.db.cursor()
		sets = [ str(k) + '=%s' for k in names ]
		sets = ', '.join(sets)
		values = tuple([ str(ins[k]) for k in names ])
		sql = 'UPDATE ' + tab + ' SET '
		sql += sets + self._where(conds)
		if limit:
			sql += ' LIMIT ' + str(limit)
		db.execute(sql, values)
		db.close()

	def delete(self, tab, conds, limit=0):
		db = self.db.cursor()
		sql = 'DELETE FROM ' + tab + self._where(conds)
		if limit:
			sql += ' LIMIT ' + str(limit)
		db.execute(sql)
		db.close()
