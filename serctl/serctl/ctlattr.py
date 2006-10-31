#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $id:$
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.dbany     import DBany
from serctl.error     import Error, EDUPL, EATTR, ENOARG, ENOREC, EINVAL, \
                             ENOATTR, ENODRA, ENOVAL
from serctl.flag      import parse_flags, new_flags, flag_syms, CND_NO_DELETED, \
                             FOR_SERWEB, cv_flags, set_deleted, CND_DELETED
from serctl.utils     import show_opts, tabprint, idx_dict, arg_attrs, \
                             col_idx, cond, full_cond, uniq, Basectl, errstr
import serctl.ctlhelp, serctl.ctldomain, serctl.ctluri


def help(*tmp):
	print """\
Usage:
        ser_attr [options...] [--] [command] [param...]

%s

Commands & parameters:
	ser_attr add    <identificator> <attr>=<value> [attr=value]...
	ser_attr purge
	ser_attr rm     <identificator> <attr> [value]
	ser_attr set    <identificator> <attr>=<value> [attr=value]...
	ser_attr show   [identificator]

Identificators:
	* global
	* uid=<uid>
	* did=<did>
	* user=<uri>
	* domain=<domain>
        * types
""" % serctl.ctlhelp.options()


def _attr_id(ident, opts):
	itmp = ident + '='
	x, id = itmp.split('=', 1)
	id = id[:-1]

	if x[0] == 'g': # global
		obj = Global_attrs(opts['DB_URI'])
		id  = None
		x   = 'g'
	elif x[:2] == 'ui': # uid
		obj = User_attrs(opts['DB_URI'])
		x   = 'u'
	elif x[:2] == 'us': #user
		obj = User_attrs(opts['DB_URI'])
		if id:
			ur  = serctl.ctluri.Uri(opts['DB_URI'])
			id = ur.get_uid(id)
		else:
			id = None
		x   = 'u' 
	elif x[:2] == 'di': #did
		obj = Domain_attrs(opts['DB_URI'])
		x   = 'd'
	elif x[:2] == 'do': #domain
		obj = Domain_attrs(opts['DB_URI'])
		if id:
			do  = serctl.ctldomain.Domain(opts['DB_URI'])
			id  = do.get_did(id)
		else:
			id = None
		x   = 'd'
	elif x[0] == 't': #types
		obj = Attr_types(opts['DB_URI'])
		id  = None
		x   = 't'
	else:
		raise Error (EINVAL, ident)
	return obj, id, x

def _attr_prep(attrs, opts):
	if not attrs:
		raise Error (ENOARG, 'identificator')
	if len(attrs) < 2:
		raise Error (ENOARG, 'attr')
	obj, id, x = _attr_id(attrs[0], opts)
	if not id and x != 'g' and x != 't':
		 raise Error (EINVAL, attrs[0])
	return obj, attrs[1:], id

def show_all(opts):
	cols, fformat, limit, rsep, lsep, astab = show_opts(opts)

	COLS = ['name', 'type', 'value', 'flags']
	if not cols:
		cols = ['did', 'uid'] + COLS
	CIDX = idx_dict(['did', 'uid'] + COLS)
	cidx = col_idx(CIDX, cols)

	lst = []

	a = User_attrs(opts['DB_URI'])
	ulist, desc = a.show(None, ['uid']+COLS, fformat, limit)
	for l in ulist:
		lst.append([''] + l) 

	a = Domain_attrs(opts['DB_URI'])
	dlist, desc = a.show(None, ['did']+COLS, fformat, limit)
	for l in dlist:
		lst.append([l[0], ''] + l[1:]) 

	a = Global_attrs(opts['DB_URI'])
	glist, desc = a.show(None, COLS, fformat, limit)
	for l in glist:
		lst.append(['GLOBAL', 'GLOBAL'] + l) 
	
	alist = []
	for row in lst:
		nr = []
		for i in cidx:
			nr.append(row[i])
		alist.append(nr)

	desc = [('did', ), ('uid', )] + list(desc)
	dsc = []
	for i in cidx:
		dsc.append(desc[i])
	tabprint(alist, dsc, rsep, lsep, astab)	

def show(identificator=None, **opts):
	if identificator is None:
		return show_all(opts)
#	COLS = ['name', 'type', 'value', 'flags']
	cols, fformat, limit, rsep, lsep, astab = show_opts(opts)

	obj, id, x = _attr_id(identificator, opts)

	if x == 'g':
		alist, desc = obj.show(None, cols, fformat, limit)
	elif x == 'u':
		alist, desc = obj.show_uid(id, cols, fformat, limit)
	elif x == 'd':
		alist, desc = obj.show_did(id, cols, fformat, limit)
	elif x == 't':
		alist, desc = obj.show(None, cols, fformat, limit)
	else:
		raise Error (EINVAL, identificator)

	tabprint(alist, desc, rsep, lsep, astab)	

def add(*attrs, **opts):
	force = opts['FORCE']
	flags = opts['FLAGS']

	obj, attrs, id = _attr_prep(attrs, opts)
	adict, alist = arg_attrs(attrs)

	if id is None:
		obj.add_many(alist, flags, force)
	else:
		obj.add_many(id, alist, flags, force)

def rm(identificator, attr, value=None, **opts):
	force = opts['FORCE']

	obj, id, x = _attr_id(identificator, opts)

	if not id and x != 'g':
		raise Error (EINVAL, identificator)

	if x == 'g':
		obj.rm(attr, value, force)
	else:
		obj.rm(id, attr, value, force)

def _set(attrs, opts):
	force = opts['FORCE']
	flags = opts['FLAGS']

	obj, attrs, id = _attr_prep(attrs, opts)
	adict, alist = arg_attrs(attrs)

	if id is None:
		obj.set_many(alist, flags, force)
	else:
		obj.set_many(id, alist, flags, force)

def set(*attrs, **opts):
	return _set(attrs, opts)

def purge(**opts):
	a = User_attrs(opts['DB_URI'])
	a.purge()
	a = Domain_attrs(opts['DB_URI'])
	a.purge()
	a = Global_attrs(opts['DB_URI'])
	a.purge()

class User_attrs(Basectl):
	TABLE = 'user_attrs'
	COLUMNS = ('uid', 'name', 'type', 'value', 'flags')
	COLIDXS = idx_dict(COLUMNS)
	FLAGIDX = COLIDXS['flags']

	def exist(self, uid, attr, value=None):
		return self.exist_cnd(cond(CND_NO_DELETED, name=attr, uid=uid, value=value))

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

	def show_uid(self, uid=None, cols=None, fformat='raw', limit=0):
		return self.show_cnd(cond(uid=uid), cols, fformat, limit)

	def add(self, uid, attr, value, flags=None, force=False):
		if not value:
			if force: return
			raise Error (ENOVAL, attr)
		at = Attr_types(self.dburi, self.db)
		dflags = at.get_default_flags(attr)
		fmask  = parse_flags(flags)
		flags  = new_flags(dflags, fmask)

		at = Attr_types(self.dburi, self.db)
		try:
			type = at.get_type(attr)
		except:
			if force: return
			raise

		# add new attr
		ins = { 'uid': uid, 'name' : attr, 'type' : type, \
		        'value' : value, 'flags' : flags }
		self.db.insert(self.TABLE, ins)

	def add_many(self, uid, alist, flags=None, force=False):
		if not force:
			at = Attr_types(self.dburi, self.db)
			for a, v in alist:
				if not at.exist(a):
					raise Error (EATTR, a)
		for a, v in alist:
			self.add(uid, a, v, flags, force)

	def rm(self, uid, attr, value=None, force=False):
		if not self.exist(uid, attr, value):
			if force: return
			if value is None:
				raise Error (ENOREC, errstr(uid=uid, attr=attr))
			else:
				raise Error (ENOREC, errstr(uid=uid, attr=attr, value=value))

		cnd, err = cond(CND_NO_DELETED, uid=uid, name=attr, value=value)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		for row in rows:
			nf = set_deleted(row[self.FLAGIDX])
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags': nf}, cnd)

		# FIX: purge hack
		self.purge()

	def set(self, uid, attr, value, flags=None, force=False):
		if self.exist(uid, attr):
			self.rm(uid, attr, force=force)
		self.add(uid, attr, value, flags, force)

	def set_many(self, uid, alist, flags=None, force=False):
		attrs = [ i[0] for i in alist ]
		attrs = uniq(attrs)
		for a in attrs:
			if self.exist(uid, a):
				self.rm(uid, a, force=force)
		for a, v in alist: 
			self.add(uid, a, v, flags, force)

	def purge(self):
		self.db.delete(self.TABLE, CND_DELETED)

class Domain_attrs(Basectl):
	TABLE = 'domain_attrs'
	COLUMNS = ('did', 'name', 'type', 'value', 'flags')
	COLIDXS = idx_dict(COLUMNS)
	FLAGIDX = COLIDXS['flags']

	def exist(self, did, attr, value=None):
		return self.exist_cnd(cond(CND_NO_DELETED, name=attr, did=did, value=value))

	def get(self, did, attr):
		cnd, err = cond(CND_NO_DELETED, name=attr, did=did)
		rows = self.db.select(self.TABLE, 'value', cnd, limit=1)
		if not rows:
			raise Error (ENODRA, did)
		return rows[0][0]

	def show_did(self, did=None, cols=None, fformat='raw', limit=0):
		return self.show_cnd(cond(did=did), cols, fformat, limit)

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

	def add(self, did, attr, value, flags=None, force=False):
		if not value:
			if force: return
			raise Error (ENOVAL, attr)
		at = Attr_types(self.dburi, self.db)
		dflags = at.get_default_flags(attr)
		fmask  = parse_flags(flags)
		flags  = new_flags(dflags, fmask)

		at = Attr_types(self.dburi, self.db)
		try:
			type = at.get_type(attr)
		except:
			if force: return
			raise

		# add new attr
		ins = { 'did': did, 'name' : attr, 'type' : type, \
		        'value' : value, 'flags' : flags }
		self.db.insert(self.TABLE, ins)

	def add_many(self, did, alist, flags=None, force=False):
		if not force:
			at = Attr_types(self.dburi, self.db)
			for a, v in alist:
				if not at.exist(a):
					raise Error (EATTR, a)

		for a, v in alist:
			self.add(did, a, v, flags, force)

	def rm(self, did, attr, value=None, force=False):
		if not self.exist(did, attr, value):
			if force: return
                        if value is None:
				raise Error (ENOREC, errstr(did=did, attr=attr))
			else:
				raise Error (ENOREC, errstr(did=did, attr=attr, value=value))

		cnd, err = cond(CND_NO_DELETED, did=did, name=attr, value=value)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		for row in rows:
			nf = set_deleted(row[self.FLAGIDX])
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags': nf}, cnd)

		# FIX: purge hack
		self.purge()

	def rm_exist(self, did, attr, value=None, force=False):
		# search for attr, return if set
		if not self.exist(did, attr): return
		self.rm(did, attr, value, force)

	def set(self, did, attr, value, flags=None, force=False):
		if self.exist(did, attr):
			self.rm(did, attr, force=force)
		self.add(did, attr, value, flags, force)

	def set_default(self, did, attr, value, flags=None, force=False):
		# search for attr, return if set
		if self.exist(did, attr): return
		self.add(did, attr, value, flags, force)

	def set_many(self, did, alist, flags=None, force=False):
		attrs = [ i[0] for i in alist ]
		attrs = uniq(attrs)
		for a in attrs:
			if self.exist(did, a):
				self.rm(did, a, force=force)
		for a, v in alist: 
			self.add(did, a, v, flags, force)

	def purge(self):
		self.db.delete(self.TABLE, CND_DELETED)



class Global_attrs(Basectl):
	TABLE = 'global_attrs'
	COLUMNS = ('name', 'type', 'value', 'flags')
	COLIDXS = idx_dict(COLUMNS)
	FLAGIDX = COLIDXS['flags']

	def exist(self, attr, value=None):
		return self.exist_cnd(cond(CND_NO_DELETED, name=attr, value=value))

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
		if not value:
			if force: return
			raise Error (ENOVAL, attr)
		at = Attr_types(self.dburi, self.db)
		dflags = at.get_default_flags(attr)
		fmask  = parse_flags(flags)
		flags  = new_flags(dflags, fmask)

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

	def add_many(self, alist, flags=None, force=False):
		if not force:
			at = Attr_types(self.dburi, self.db)
			for a, v in alist:
				if not at.exist(a):
					raise Error (EATTR, a)

		for a, v in alist:
			self.add(a, v, flags, force)

	def rm(self, attr, value=None, force=False):
		if not self.exist(attr, value):
			if force: return
                        if value is None:
				raise Error (ENOREC, errstr(attr=attr))
			else:
				raise Error (ENOREC, errstr(attr=attr, value=value))

		cnd, err = cond(CND_NO_DELETED, name=attr, value=value)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		for row in rows:
			nf = set_deleted(row[self.FLAGIDX])
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags': nf}, cnd)

		# FIX: purge hack
		self.purge()

	def set(self, attr, value, flags=None, force=False):
		if self.exist(attr):
			self.rm(attr, force=force)
		self.add(attr, value, flags, force)

	def set_many(self, alist, flags=None, force=False):
		attrs = [ i[0] for i in alist ]
		attrs = uniq(attrs)
		for a in attrs:
			if self.exist(a):
				self.rm(a, force=force)
		for a, v in alist: 
			self.add(a, v, flags, force)

	def purge(self):
		self.db.delete(self.TABLE, CND_DELETED)

class Attr_types(Basectl):
	TABLE = 'attr_types'
	COLUMNS = ('name', 'rich_type', 'raw_type', 'priority', 'ordering',
	           'type_spec', 'flags', 'default_flags', 'description')
	COLIDXS = idx_dict(COLUMNS)
	FLAGIDX = COLIDXS['flags']


	def exist(self, attr):
		return self.exist_cnd(cond(CND_NO_DELETED, name=attr))

	def show(self, attr=None, cols=None, fformat='raw', limit=0):
		return self.show_cnd(cond(name=attr), cols, fformat, limit)

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
