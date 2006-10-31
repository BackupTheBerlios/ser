#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: dbsql.py,v 1.7 2006/10/31 19:40:15 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# Base class for SQL databases.

from serctl.dbbase import DBbase
from serctl.opsql  import where
from serctl.uri    import USER, PASS, HOST, PORT, DB

class DBsql(DBbase):

	def _connect(self, user, password, host, port, db):
	# This function should be implemented in db<driver>.py
	# See dbmysql.py for example.
		pass

	def __init__(self, puri):
		self.db = self._connect(puri[USER], puri[PASS], puri[HOST], \
		                        puri[PORT], puri[DB])

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
		self.db.commit()
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
			type = None	# FIX: not-implemented
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
		self.db.commit()
		db.close()

	def delete(self, tab, conds, limit=0):
		db = self.db.cursor()
		sql = 'DELETE FROM ' + tab + self._where(conds)
		if limit:
			sql += ' LIMIT ' + str(limit)
		db.execute(sql)
		self.db.commit()
		db.close()
