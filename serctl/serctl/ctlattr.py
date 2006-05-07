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
from serctl.error     import Error, EDUPL, EATTR, ENOARG, ENOREC, EINVAL
from serctl.flag      import parse_flags, new_flags, flag_syms, CND_NO_DELETED, \
                             FOR_SERWEB, cv_flags, set_deleted, CND_DELETED
from serctl.utils     import show_opts, tabprint, idx_dict, arg_attrs, \
                             col_idx, cond, full_cond, uniq, Basectl
import serctl.ctlhelp


def help(*tmp):
	print """\
Usage:
        ser_attr [options...] [--] [command] [param...]

%s

Commands & parameters:
	ser_attr add    global | uid=<uid> | did=<did> <attr>=<value> [attr=value]...
	ser_attr attr   [attr]
	ser_attr change global | uid=<uid> | did=<did> <attr>=<value> [attr=value]...
	ser_attr purge
	ser_attr rm     global | uid=<uid> | did=<did> <attr> [attr...]
	ser_attr set    global | uid=<uid> | did=<did> <attr>=<value> [attr=value]...
	ser_attr show   [global | uid=<uid> | did=<did>]
""" % serctl.ctlhelp.options()

def _attr_prep(attrs, opts):
	if not attrs:
		raise Error (ENOARG, 'global|uid|did')
	if len(attrs) < 2:
		raise Error (ENOARG, 'attr')
	if attrs[0] == 'global':
		obj = Global_attrs(opts['DB_URI'])
		xid = None
	elif attrs[0][:4] == 'uid=':
		obj = User_attrs(opts['DB_URI'])
		xid = attrs[0][4:]
	elif attrs[0][:4] == 'did=':
		obj = Domain_attrs(opts['DB_URI'])
		xid = attrs[0][4:]
	else:
		raise Error (EINVAL, attrs[0])
	return obj, attrs[1:], xid

def OLD_show(*attrs, **opts):
	cols, fformat, limit, rsep, lsep, astab = show_opts(opts)

	COLS = ['name', 'type', 'value', 'flags']

	lst = []

	a = User_attrs(opts['DB_URI'])
	ulist, desc = a.show(attrs, ['uid']+COLS, fformat, limit)
	for l in ulist:
		lst.append([''] + l) 

	a = Domain_attrs(opts['DB_URI'])
	dlist, desc = a.show(attrs, ['did']+COLS, fformat, limit)
	for l in dlist:
		lst.append([l[0], ''] + l[1:]) 

	a = Global_attrs(opts['DB_URI'])
	glist, desc = a.show(attrs, COLS, fformat, limit)
	for l in glist:
		lst.append(['GLOBAL', 'GLOBAL'] + l) 

	desc = [('did', ), ('uid', )] + list(desc)
	tabprint(lst, desc, rsep, lsep, astab)	

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

def show(global_uid_did=None, **opts):
	if global_uid_did is None:
		return show_all(opts)
	COLS = ['name', 'type', 'value', 'flags']
	cols, fformat, limit, rsep, lsep, astab = show_opts(opts)
	if global_uid_did == 'global':
		obj = Global_attrs(opts['DB_URI'])
		alist, desc = obj.show(None, cols, fformat, limit)
	elif global_uid_did[:4] == 'uid=':
		obj = User_attrs(opts['DB_URI'])
		uid=global_uid_did[4:]
		alist, desc = obj.show_uid(uid, cols, fformat, limit)
	elif global_uid_did[:4] == 'did=':
		obj = Domain_attrs(opts['DB_URI'])
		did=global_uid_did[4:]
		alist, desc = obj.show_did(did, cols, fformat, limit)
	else:
		raise Error (EINVAL, global_uid_did)

	tabprint(alist, desc, rsep, lsep, astab)	

def add(*attrs, **opts):
	force = opts['FORCE']
	flags = opts['FLAGS']

	if not attrs:
		raise Error (ENOARG, 'attr')

	a, attrs, xid = _attr_prep(attrs, opts)
	adict, alist = arg_attrs(attrs)

	if xid is None:
		a.add_many(adict, flags, force)
	else:
		a.add_many(xid, adict, flags, force)

def rm(*attrs, **opts):
	force = opts['FORCE']

	a, attrs, xid = _attr_prep(attrs, opts)
	if xid is None:
		a.rm_many(attrs, force)
	else:
		a.rm_many(xid, attrs, force)

def change(*attrs, **opts):
	force = opts['FORCE']
	flags = opts['FLAGS']

	a, attrs, xid = _attr_prep(attrs, opts)
	adict, alist = arg_attrs(attrs)

	if xid is None:
		a.change_many(adict, flags, force)
	else:
		a.change_many(xid, adict, flags, force)

def set(*attrs, **opts):
	force = opts['FORCE']
	flags = opts['FLAGS']

	a, attrs, xid = _attr_prep(attrs, opts)
	adict, alist = arg_attrs(attrs)

	if xid is None:
		a.set_many(adict, flags, force)
	else:
		a.set_many(xid, adict, flags, force)


def purge(**opts):
	a = User_attrs(opts['DB_URI'])
	a.purge()
	a = Domain_attrs(opts['DB_URI'])
	a.purge()
	a = Global_attrs(opts['DB_URI'])
	a.purge()

def attrs(attr=None, **opts):
	cols, fformat, limit, rsep, lsep, astab = show_opts(opts)
	if opts['COLUMNS'] is None:
		cols = ['name']
	obj = Attr_types(opts['DB_URI'])
	alist, desc = obj.show(attr, cols, fformat, limit)
	tabprint(alist, desc, rsep, lsep, astab)	
	
class User_attrs(Basectl):
	TABLE = 'user_attrs'
	COLUMNS = ('uid', 'name', 'type', 'value', 'flags')
	COLIDXS = idx_dict(COLUMNS)
	FLAGIDX = COLIDXS['flags']

	def exist(self, uid, attr):
		return self.exist_cnd(cond(CND_NO_DELETED, name=attr, uid=uid))

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
		at = Attr_types(self.dburi, self.db)
		dflags = at.get_default_flags(attr)
		fmask  = parse_flags(flags)
		flags  = new_flags(dflags, fmask)

		if self.exist(uid, attr):
			if force: return
			raise Error (EDUPL, err)

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

	def add_many(self, uid, adict, flags=None, force=False):
		if not force:
			for a in adict.keys():
				if self.exist(uid, a):
					raise Error (EDUPL, a)
		for a, v in adict.items():
			self.add(uid, a, v, flags, force)

	def rm(self, uid, attr, force=False):
		if not self.exist(uid, attr):
			if force: return
			raise Error (ENOREC, attr)

		cnd, err = cond(CND_NO_DELETED, uid=uid, name=attr)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		for row in rows:
			nf = set_deleted(row[self.FLAGIDX])
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags': nf}, cnd)

	def rm_many(self, uid, alist, force=False):
		alist = uniq(alist)
		if not force:
			for a in alist:
				if not self.exist(uid, a):
					raise Error (ENOREC, a)
		for a in alist:
			self.rm(uid, a, force)

	def change(self, uid, attr, value, flags=None, force=False):
		fmask = parse_flags(flags)
		nflags = new_flags(0, fmask)

		if not self.exist(uid, attr):
			if force: return
			raise Error (ENOREC, err)

		at = Attr_types(self.dburi, self.db)
		try:
			type = at.get_type(attr)
		except:
			if force: return
			raise

		cnd, err = cond(CND_NO_DELETED, uid=uid, name=attr)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		for row in rows:
			nf = new_flags(row[self.FLAGIDX], fmask)
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'value':value, 'flags':nf}, cnd)

	def change_many(self, uid, adict, flags=None, force=False):
		if not force:
			for a in adict.keys():
				if not self.exist(uid, a):
					raise Error (ENOREC, a)
		for a, v in adict.items():
			self.change(uid, a, v, flags, force)

	def set(self, uid, attr, value, flags=None, force=False):
		if self.exist(uid, attr):
			self.change(uid, attr, value, flags, force)
		else:
			self.add(uid, attr, value, flags, force)

	def set_many(self, uid, adict, flags=None, force=False):
		for a, v in adict.items():
			self.set(uid, a, v, flags, force)

	def purge(self):
		self.db.delete(self.TABLE, CND_DELETED)

class Domain_attrs(Basectl):
	TABLE = 'domain_attrs'
	COLUMNS = ('did', 'name', 'type', 'value', 'flags')
	COLIDXS = idx_dict(COLUMNS)
	FLAGIDX = COLIDXS['flags']

	def exist(self, did, attr):
		return self.exist_cnd(cond(CND_NO_DELETED, name=attr, did=did))

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
		at = Attr_types(self.dburi, self.db)
		dflags = at.get_default_flags(attr)
		fmask  = parse_flags(flags)
		flags  = new_flags(dflags, fmask)

		if self.exist(did, attr):
			if force: return
			raise Error (EDUPL, err)

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

	def add_many(self, did, adict, flags=None, force=False):
		if not force:
			for a in adict.keys():
				if self.exist(did, a):
					raise Error (EDUPL, a)
		for a, v in adict.items():
			self.add(did, a, v, flags, force)

	def rm(self, did, attr, force=False):
		if not self.exist(did, attr):
			if force: return
			raise Error (ENOREC, attr)

		cnd, err = cond(CND_NO_DELETED, did=did, name=attr)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		for row in rows:
			nf = set_deleted(row[self.FLAGIDX])
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags': nf}, cnd)

	def rm_many(self, did, alist, force=False):
		alist = uniq(alist)
		if not force:
			for a in alist:
				if not self.exist(did, a):
					raise Error (ENOREC, a)
		for a in alist:
			self.rm(did, a, force)

	def change(self, did, attr, value, flags=None, force=False):
		fmask = parse_flags(flags)
		nflags = new_flags(0, fmask)

		if not self.exist(did, attr):
			if force: return
			raise Error (ENOREC, err)

		at = Attr_types(self.dburi, self.db)
		try:
			type = at.get_type(attr)
		except:
			if force: return
			raise

		cnd, err = cond(CND_NO_DELETED, did=did, name=attr)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		for row in rows:
			nf = new_flags(row[self.FLAGIDX], fmask)
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'value':value, 'flags':nf}, cnd)

	def change_many(self, did, adict, flags=None, force=False):
		if not force:
			for a in adict.keys():
				if not self.exist(did, a):
					raise Error (ENOREC, a)
		for a, v in adict.items():
			self.change(did, a, v, flags, force)

	def set(self, did, attr, value, flags=None, force=False):
		if self.exist(did, attr):
			self.change(did, attr, value, flags, force)
		else:
			self.add(did, attr, value, flags, force)

	def set_many(self, did, adict, flags=None, force=False):
		for a, v in adict.items():
			self.set(did, a, v, flags, force)

	def purge(self):
		self.db.delete(self.TABLE, CND_DELETED)


	def exist_dra(self, did):
		cnd, err = cond(CND_NO_DELETED, name='digest_realm', value=did)
		rows = self.db.select(self.TABLE, 'did', cnd, limit=1)
		return rows != []

	def add_dra(self, did, domain):
		at = Attr_types(self.dburi, self.db)
		flags = at.get_default_flags('digest_realm')
		ins = {'did': did, 'name': 'digest_realm', 'value': domain, \
		  'type': 2, 'flags': flags }
		self.db.insert(self.TABLE, ins)

	def set_dra_if_not_set(self, did, domain):
		# search for digest realm attr, return if set
		cnd, err = cond(CND_NO_DELETED, did=did, name='digest_realm')
		rows = self.db.select(self.TABLE, 'did', cnd, limit=1)
		if rows:
			return
		self.add_dra(did, domain)

	def rm_dra(self, did):
		cnd, err = cond(CND_NO_DELETED, did=did, name='digest_realm')
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		for row in rows:
			nf = set_deleted(row[self.FLAGIDX])
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags': nf}, cnd)
	def purge(self):
		self.db.delete(self.TABLE, CND_DELETED)



class Global_attrs(Basectl):
	TABLE = 'global_attrs'
	COLUMNS = ('name', 'type', 'value', 'flags')
	COLIDXS = idx_dict(COLUMNS)
	FLAGIDX = COLIDXS['flags']

	def exist(self, attr):
		return self.exist_cnd(cond(CND_NO_DELETED, name=attr))

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

class Attr_types(Basectl):
	TABLE = 'attr_types'
	COLUMNS = ('name', 'rich_type', 'raw_type', 'priority', 'ordering',
	           'type_spec', 'flags', 'default_flags', 'description')
	COLIDXS = idx_dict(COLUMNS)
	FLAGIDX = COLIDXS['flags']


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
