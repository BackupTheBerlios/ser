#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $$
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.dbany     import DBany
from serctl.error     import Error, EDUPL, EATTR, ENOARG, ENOREC
from serctl.flag      import parse_flags, new_flags, flag_syms, CND_NO_DELETED, \
                             FOR_SERWEB, cv_flags, set_deleted, CND_DELETED
from serctl.utils     import show_opts, tabprint, arg_pairs, idx_dict, \
                             col_idx, cond, full_cond, uniq
import serctl.ctlhelp


def help(*tmp):
	print """\
Usage:
        ser_attr [options...] [--] [command] [param...]

%s

Commands & parameters:
	ser_uri add    <attr> <value> [attr value]...
	ser_uri change <attr> <value> [attr value]...
	ser_uri purge
	ser_uri rm     <attr> [attr...]
	ser_uri set    <attr> <value> [attr value]...
	ser_uri show   [attr...]
""" % serctl.ctlhelp.options()

def show(*attrs, **opts):
	cols, fformat, limit, rsep, lsep, astab = show_opts(opts)

	a = Attr(opts['DB_URI'])
	alist, desc = a.show(attrs, cols, fformat, limit)

	tabprint(alist, desc, rsep, lsep, astab)

def add(*attrs, **opts):
	force = opts['FORCE']
	flags = opts['FLAGS']

	if not attrs:
		raise Error (ENOARG, 'attr')

	a = Attr(opts['DB_URI'])

	adict, alist = arg_pairs(attrs)

	a.add_many(adict, flags, force)

def rm(*attrs, **opts):
	force = opts['FORCE']
	if not attrs:
		raise Error (ENOARG, 'attr')
	a = Attr(opts['DB_URI'])

	a.rm_many(attrs, force)

def change(*attrs, **opts):
	force = opts['FORCE']
	flags = opts['FLAGS']

	if not attrs:
		raise Error (ENOARG, 'attr')

	a = Attr(opts['DB_URI'])

	adict, alist = arg_pairs(attrs)

	a.change_many(adict, flags, force)

def set(*attrs, **opts):
	force = opts['FORCE']
	flags = opts['FLAGS']

	if not attrs:
		raise Error (ENOARG, 'attr')

	a = Attr(opts['DB_URI'])

	adict, alist = arg_pairs(attrs)

	a.set_many(adict, flags, force)


def purge(**opts):
	a = Attr(opts['DB_URI'])
	a.purge()

class Attr:
	TABLE = 'global_attrs'
	COLUMNS = ('name', 'type', 'value', 'flags')
	COLIDXS = idx_dict(COLUMNS)
	FLAGIDX = COLIDXS['flags']


	def __init__(self, dburi, db=None):
		self.Domain = serctl.ctldomain.Domain
		self.User   = serctl.ctluser.User
		self.dburi  = dburi
		if db is not None:
			self.db = db
		else:
			self.db = DBany(dburi)

	def exist(self, attr):
		cnd, err = cond(CND_NO_DELETED, name=attr)
		rows = self.db.select(self.TABLE, 'name', cnd, limit=1)
		return rows != []

	def show(self, attrs=[], cols=None, fformat='raw', limit=0):
		if not cols:
			cols = self.COLUMNS
		cidx = col_idx(self.COLIDXS, cols)
		

		if not attrs:
			attrs=[None]
		rows = []
		for attr in attrs:
			cnd, err = cond(name=attr)
			rows += self.db.select(self.TABLE, self.COLUMNS, cnd, limit)
		if limit > 0:
			rows = rows[:limit]

		new_rows = []
		for row in rows:
			row[self.FLAGIDX] = cv_flags(fformat, row[self.FLAGIDX])
			new_row = []
			for i in cidx:
				new_row.append(row[i])
			new_rows.append(new_row)
		desc = self.db.describe(self.TABLE)
		desc = [ desc[i] for i in cols ]
		return new_rows, desc

	def add(self, attr, value, flags=None, force=False):
		at = Attr_types(self.dburi, self.db)
		dflags = at.get_default_flags(attr)
		fmask  = parse_flags(flags)
		flags  = new_flags(dflags, fmask)

		if self.exist(attr):
			if force: return
			raise Error (EDUPL, err)

		at = Attr_types(self.dburi, self.db)
		try:
			type = at.get_type(attr)
		except:
			if force: return
			raise

		# add new attr
		ins = { 'name' : attr, 'type' : type, 'value' : value, \
			'flags' : flags }
		self.db.insert(self.TABLE, ins)

	def add_many(self, adict, flags=None, force=False):
		if not force:
			for a in adict.keys():
				if self.exist(a):
					raise Error (EDUPL, a)
		for a, v in adict.items():
			self.add(a, v, flags, force)

	def rm(self, attr, force=False):
		if not self.exist(attr):
			if force: return
			raise Error (ENOREC, attr)

		cnd, err = cond(CND_NO_DELETED, name=attr)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		for row in rows:
			nf = set_deleted(row[self.FLAGIDX])
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags': nf}, cnd)

	def rm_many(self, alist, force=False):
		alist = uniq(alist)
		if not force:
			for a in alist:
				if not self.exist(a):
					raise Error (ENOREC, a)
		for a in alist:
			self.rm(a, force)

	def change(self, attr, value, flags=None, force=False):
		fmask = parse_flags(flags)
		nflags = new_flags(0, fmask)

		if not self.exist(attr):
			if force: return
			raise Error (ENOREC, err)

		at = Attr_types(self.dburi, self.db)
		try:
			type = at.get_type(attr)
		except:
			if force: return
			raise

		cnd, err = cond(CND_NO_DELETED, name=attr)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		for row in rows:
			nf = new_flags(row[self.FLAGIDX], fmask)
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'value':value, 'flags':nf}, cnd)

	def change_many(self, adict, flags=None, force=False):
		if not force:
			for a in adict.keys():
				if not self.exist(a):
					raise Error (ENOREC, a)
		for a, v in adict.items():
			self.change(a, v, flags, force)

	def set(self, attr, value, flags=None, force=False):
		if self.exist(attr):
			self.change(attr, value, flags, force)
		else:
			self.add(attr, value, flags, force)

	def set_many(self, adict, flags=None, force=False):
		for a, v in adict.items():
			self.set(a, v, flags, force)

	def purge(self):
		self.db.delete(self.TABLE, CND_DELETED)



class Attr_types:
	TABLE = 'attr_types'
	COLUMNS = ('name', 'rich_type', 'raw_type', 'priority', 'ordering',
	           'type_spec', 'flags', 'default_flags', 'description')
	COLIDXS = idx_dict(COLUMNS)
	FLAGIDX = COLIDXS['flags']


	def __init__(self, dburi, db=None):
		self.Domain = serctl.ctldomain.Domain
		self.User   = serctl.ctluser.User
		self.dburi  = dburi
		if db is not None:
			self.db = db
		else:
			self.db = DBany(dburi)

	def get_type(self, attr):
		cnd, err = cond(CND_NO_DELETED, name=attr)
		rows = self.db.select(self.TABLE, 'raw_type', cnd, limit=1)
		if not rows:
			raise Error (EATTR, attr)
		return int(rows[0][0])

	def get_default_flags(self, attr):
		cnd, err = cond(CND_NO_DELETED, name=attr)
		rows = self.db.select(self.TABLE, 'default_flags', cnd, limit=1)
		if not rows:
			raise Error (EATTR, attr)
		return int(rows[0][0])
