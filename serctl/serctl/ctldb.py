#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctldb.py,v 1.5 2006/04/27 22:32:20 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.dbany   import DBany, DSC_NAME, DSC_DEFAULT
from serctl.error   import Error, EALL, ECOLSPEC
from serctl.utils   import show_opts, arg_pairs, tabprint
import serctl.ctlhelp

def help(*tmp):
	print """
Usage:
	ser_db [options...] [--] add  <table_name> [column=value] ...
	ser_db [options...] [--] rm   <table_name> [column=value] ...
	ser_db [options...] [--] show <table_name> [column=value] ...

%s

Note:
	This is a simple and stupid command. It's don't handle NULL value.
	Column values are allways strings. Condition for rm or show is created
	as list of column=value pairs simply joined together with AND operator.
""" % serctl.ctlhelp.options()

def _columns(args):
	l = []
	for arg in args:
		x = arg.split('=', 1)
		if len(x) < 2:
			raise Error (ECOLSPEC, arg)
		l += [x[0], x[1]]
	return arg_pairs(l)

def show(table_name, *args, **opts):
	clist = _columns(args)[1]
	cols, fformat, limit, rsep, lsep,  astab = show_opts(opts)

	u = Db(opts['DB_URI'])
	data, desc = u.show(table_name, cols, clist, limit)

	tabprint(data, desc, rsep, lsep, astab)

def add(table_name, *args, **opts):
	cdict = _columns(args)[0]

	u = Db(opts['DB_URI'])
	u.add(table_name, cdict)

def rm(table_name, *args, **opts):
	if (not args) and (not opts['FORCE']):
		raise Error (EALL, )

	clist = _columns(args)[1]
	limit = opts['LIMIT']

	u = Db(opts['DB_URI'])
	u.rm(table_name, clist, limit)

class Db:

	def __init__(self, dburi):
		self.db = DBany(dburi)

	def _simple_and_cond(self, cond):
		if not cond:
			return None
		cnd = ['and', 1]
		for c in cond:
			cnd.append(('=', c[0], c[1]))
		return cnd

	def show(self, tab, cols, cond, limit=0):
		desc = self.db.describe(tab)
		if not cols:
			cols = desc.keys()
		head = [ desc[i] for i in cols ]
		cond = self._simple_and_cond(cond)
		return self.db.select(tab, cols, cond, limit), head

	def add(self, tab, ins):
		self.db.insert(tab, ins)

	def rm(self, tab, cond, limit=0):
		cond = self._simple_and_cond(cond)
		self.db.delete(tab, cond, limit)
