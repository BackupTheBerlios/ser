#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctluser.py,v 1.10 2006/10/31 19:40:15 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.dbany   import DBany
from serctl.error   import Error, ENOARG, EINVAL, EDUPL, ENOCOL, EDOMAIN, ENOREC, \
                           EMULTICANON, EUSER
from serctl.flag    import parse_flags, new_flags, clear_canonical, set_canonical, \
                           is_canonical, set_deleted, flag_syms, CND_NO_DELETED, \
                           CND_DELETED, CND_CANONICAL, LOAD_SER, FOR_SERWEB, cv_flags
from serctl.utils   import show_opts, tabprint, arg_pairs, idx_dict, timestamp, \
                           no_all, col_idx, cond, full_cond
import serctl.ctlhelp, serctl.ctlcred, serctl.ctluri

def help(*tmp):
	print """\
Usage:
	ser_user [options...] [--] [command] [param...]

%s

Commands & parameters:
	ser_user add     <uid> [uid...]
	ser_user change  [uid] [-F flags]
	ser_user disable [uid]
	ser_user enable  [uid]
	ser_user rm      [uid]
	ser_user purge
	ser_user show    [uid]
""" % serctl.ctlhelp.options()


def show(uid=None, **opts):
	cols, fformat, limit, rsep, lsep, astab = show_opts(opts)

	u = User(opts['DB_URI'])
	uri_list, desc = u.show(uid, cols, fformat, limit)

	tabprint(uri_list, desc, rsep, lsep, astab)

def add(*uids, **opts):
	force = opts['FORCE']
	flags = opts['FLAGS']

	if not uids:
		raise Error (ENOARG, 'uid')

	u = User(opts['DB_URI'])

	if not force:
		for uid in uids:
			if u.exist(uid):
				raise Error (EDUPL, uid)

	for uid in uids:
		u.add(uid, flags, force)

def rm(uid=None, **opts):
	no_all(opts, uid)
	force = opts['FORCE']

	u = User(opts['DB_URI'])
	u.rm(uid, force=force)

def _change(uid, opts):
	no_all(opts, uid)

	force = opts['FORCE']
	flags = opts['FLAGS']

	u = User(opts['DB_URI'])
	u.change(uid, flags=flags, force=force)

def change(uid=None, **opts):
	return _change(uid, opts)

def enable(uid=None, **opts):
	opts['FLAGS'] = '-d'
	return _change(uid, opts)

def disable(uid=None, **opts):
	opts['FLAGS'] = '+d'
	return _change(uid, opts)

def purge(**opts):
	u = User(opts['DB_URI'])
	u.purge()

class User:
	TABLE = 'user_attrs'
	COLUMNS = ('uid', 'name', 'value', 'type',  'flags')
	COLIDXS = idx_dict(COLUMNS)
	FLAGIDX = COLIDXS['flags']

	def __init__(self, dburi, db=None):
		self.Uri   = serctl.ctluri.Uri
		self.Cred  = serctl.ctlcred.Cred
		self.dburi = dburi
		if db is not None:
			self.db = db
		else:
			self.db = DBany(dburi)

	def default_flags(self):
		return '0'

	def is_used(self, uid):
		ur = self.Uri(self.dburi, self.db)
		cr = self.Cred(self.dburi, self.db)
		return ur.exist_uid(uid) or cr.exist_uid(uid)

	def get(self, uid):
		cnd, err = cond(CND_NO_DELETED, uid=uid)
		rows = self.db.select(self.TABLE, 'uid', cnd)
		if not rows:
			raise Error (ENOREC, err)
		return [ i[0] for i in rows ]

	def exist(self, uid):
		cnd, err = cond(CND_NO_DELETED, uid=uid)
		rows = self.db.select(self.TABLE, 'uid', cnd, limit=1)
		return rows != []

	def _try_rm_orphans(self, uid):
		ur = self.Uri(self.dburi, self.db)
		cr = self.Cred(self.dburi, self.db)
		try:
			ur.rm_uid(uid)
		except:
			pass
		try:
			cr.rm_realm(uid)
		except:
			pass

	def show(self, uid=None, cols=None, fformat='raw', limit=0):
		if not cols:
			cols = self.COLUMNS
		cidx = col_idx(self.COLIDXS, cols)

		cnd, err = cond(uid=uid)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd, limit)
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

	def add(self, uid, flags=None, force=False):
		dflags = self.default_flags()
		fmask  = parse_flags(flags)
		flags  = new_flags(dflags, fmask)

		# exist uid?
		cnd, err = cond(CND_NO_DELETED, uid=uid)
		rows = self.db.select(self.TABLE, 'uid', cnd, limit=1)
		if rows:
			if force: return
			raise Error (EDUPL, uid)

		# user add
		ins = {'uid': uid, 'name': 'datetime_created', \
		  'value': timestamp(), 'type': 2, 'flags': flags }
		self.db.insert(self.TABLE, ins)

	def rm(self, uid=None, force=False):
		if self.is_used(uid):
			if force:
				self._try_rm_orphans(uid)
			else:
				raise Error (EUSER, uid)

		# rm all uids (FIX: or only datetime_created?)
		cnd, err = cond(CND_NO_DELETED, uid=uid)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		if not rows and not force:
			raise Error (ENOREC, err)
		for row in rows:
			nf = set_deleted(row[self.FLAGIDX])
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags': nf}, cnd)

	def change(self, uid=None, flags=None, force=False):
		fmask = parse_flags(flags)

		cnd, err = cond(CND_NO_DELETED, uid=uid)

		# update flags
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		if not rows and not force:
			raise Error (ENOREC, err)
		for row in rows:
			nf = new_flags(row[self.FLAGIDX], fmask)
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags':nf}, cnd)

	def purge(self):
		self.db.delete(self.TABLE, CND_DELETED)
