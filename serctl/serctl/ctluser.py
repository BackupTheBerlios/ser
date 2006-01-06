#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctluser.py,v 1.2 2006/01/06 14:59:43 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# Created:     2005/11/30
# Last update: 2006/01/06

from ctlcred import Cred
from ctluri  import Uri
from dbany   import DBany
from error   import Error, ENOARG, EINVAL, EDUPL, ENOCOL, EDOMAIN, ENOREC, \
                    EMULTICANON, EUSER
from flag    import parse_flags, new_flags, clear_canonical, set_canonical, \
                    is_canonical, set_deleted, flag_syms, CND_NO_DELETED, \
                    CND_DELETED, CND_CANONICAL, LOAD_SER, FOR_SERWEB
from options import CMD_ADD, CMD_CANONICAL, CMD_DISABLE, CMD_ENABLE, CMD_HELP, \
                    CMD_CHANGE, CMD_RM, CMD_SHOW, CMD_PURGE, \
                    OPT_DATABASE, OPT_FORCE, OPT_LIMIT, OPT_FLAGS
from utils   import show_opts, tabprint, arg_pairs, idx_dict, timestamp, no_all

def main(args, opts):
	cmd = args[2]
	db  = opts[OPT_DATABASE]

	if   cmd == CMD_ADD:
		ret = add(db, args[3:], opts)
	elif cmd == CMD_ENABLE:
		ret = enable(db, args[3:], opts)
	elif cmd == CMD_DISABLE:
		ret = disable(db, args[3:], opts)
	elif cmd == CMD_CHANGE:
		ret = change(db, args[3:], opts)
	elif cmd == CMD_RM:
		ret = rm(db, args[3:], opts)
	elif cmd == CMD_SHOW:
		ret = show(db, args[3:], opts)
	elif cmd == CMD_PURGE:
		ret = purge(db, args[3:], opts)
	else:
		raise Error (EINVAL, cmd)
	return ret

def help(args, opts):
	return """\
Usage:
	ser_user [options...] [--] [command] [param...]

Commands & parameters:
	ser_user add     <user>
	ser_user change  [user] [-F flags]
	ser_user disable [user]
	ser_user enable  [user]
	ser_user rm      [user]
	ser_user purge
	ser_user show    [user]
"""

def _get_uid(args, mandatory=True):
	try:
		uid = args[0]
	except:
		if mandatory:
			raise Error (ENOARG, 'uid')
		else:
			uid = None
	return uid

def show(db, args, opts):
	uid = _get_uid(args, False)

	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	u = User(db)
	uri_list, desc = u.show(uid, cols=cols, raw=numeric, limit=limit)

	tabprint(uri_list, desc, rsep, lsep, astab)

def add(db, args, opts):
	uid = _get_uid(args)

	force = opts.has_key(OPT_FORCE)
	flags = opts.get(OPT_FLAGS)

	u = User(db)
	u.add(uid, flags=flags, force=force)

def change(db, args, opts):
	uid = _get_uid(args, False)

	force = opts.has_key(OPT_FORCE)
	flags = opts.get(OPT_FLAGS)

	u = User(db)
	u.change(uid, flags=flags, force=force)

def rm(db, args, opts):
	uid = _get_uid(args, False)

	force = opts.has_key(OPT_FORCE)

	u = User(db)
	u.rm(uid, force=force)

def enable(db, args, opts):
	uid = _get_uid(args, False)

	force = opts.has_key(OPT_FORCE)

	u = User(db)
	u.enable(uid, force=force)

def disable(db, args, opts):
	uid = _get_uid(args, False)

	force = opts.has_key(OPT_FORCE)

	u = User(db)
	u.disable(uid, force=force)

def purge(db, args, opts):
	u = User(db)
	u.purge()

class User:
	T_UATTR = 'user_attrs'
	C_UATTR = ('uid', 'name', 'value', 'type',  'flags')
	I_UATTR = idx_dict(C_UATTR)
	F_UATTR = I_UATTR['flags']

	T_URI   = 'uri'
	T_CRED  = 'credentials'

	def __init__(self, dburi):
		self.dburi = dburi
		self.db = DBany(dburi)

	def _col_idx(self, cols):
		idx = []
		for col in cols:
			try:
				i = self.I_UATTR[col]
			except KeyError:
				raise Error (ENOCOL, col)
			idx.append(i)
		return tuple(idx)

	def _full_cond(self, row):
		cnd = ['and', CND_NO_DELETED]
		for i in range(len(self.C_UATTR)):
			cnd.append(('=', self.C_UATTR[i], row[i]))
		return tuple(cnd)

	def _cond(self, uid=None, all=False):
		cnd =  ['and']
		if not all:
			cnd.append(CND_NO_DELETED)
		err = []
		if uid is not None:
			cnd.append(('=', 'uid', uid))
			err.append('uid=' + uid)
		err = ' '.join(err)
		return (cnd, err)

	def show(self, uid=None, cols=None, raw=False, limit=0):
		if not cols:
			cols = self.C_UATTR
		cidx = self._col_idx(cols)

		cnd, err = self._cond(uid, all=True)
		rows = self.db.select(self.T_UATTR, self.C_UATTR, cnd, limit)
		new_rows = []
		for row in rows:
			if not raw:
				row[self.F_UATTR] = flag_syms(row[self.F_UATTR])
			new_row = []
			for i in cidx:
				new_row.append(row[i])
			new_rows.append(new_row)
		desc = self.db.describe(self.T_UATTR)
		desc = [ desc[i] for i in cols ]
		return new_rows, desc

	def default_flags(self):
		return '0'

	def add(self, uid, flags=None, force=False):
		dflags = self.default_flags()
		fmask  = parse_flags(flags)
		flags  = new_flags(dflags, fmask)

		# exist uid?
		cnd, err = self._cond(uid)
		rows = self.db.select(self.T_UATTR, 'uid', cnd, limit=1)
		if rows:
			if not force:
				raise Error (EDUPL, uid)
			self.rm(uid, force=True)

		# user add
		ins = {'uid': uid, 'name': 'datetime_created', \
		  'value': timestamp(), 'type': 2, 'flags': flags }
		self.db.insert(self.T_UATTR, ins)

	def change(self, uid=None, flags=None, force=False):
		fmask = parse_flags(flags)
		nflags = new_flags(0, fmask)

		cond, error = self._cond(uid)

		# update flags
		rows = self.db.select(self.T_UATTR, self.C_UATTR, cond)
		if not rows and not force:
			raise Error (ENOREC, error)
		for row in rows:
			nf = new_flags(row[self.F_UATTR], fmask)
			cnd = self._full_cond(row)
			self.db.update(self.T_UATTR, {'flags':nf}, cnd)

	def rm(self, uid=None, force=False):
		udel = cdel = False

		# uid in uri?
		cnd = ('and', ('=', 'uid', uid), CND_NO_DELETED)
		rows = self.db.select(self.T_URI, 'uid', cnd, limit=1)
		if rows:
			if not force:
				raise Error (EUSER, uid)
			udel = True
			
		# uid in cred?
		cnd = ('and', ('=', 'uid', uid), CND_NO_DELETED)
		rows = self.db.select(self.T_CRED, 'uid', cnd, limit=1)
		if rows:
			if not force:
				raise Error (EUSER, uid)
			cdel = True

		# delete in uri and cred
		if udel:
			udb = Uri(self.dburi)
			udb.rm(uid=uid, force=True)
			del(udb)
		if cdel:
			cdb = Cred(self.dburi)
			cdb.rm(uid=uid, force=True)
			del(cdb)

		# rm all uids (FIX: or only datetime_created?)
		cnd, err = self._cond(uid)
		rows = self.db.select(self.T_UATTR, self.C_UATTR, cnd)
		if not rows and not force:
			raise Error (ENOREC, err)
		for row in rows:
			nf = set_deleted(row[self.F_UATTR])
			cnd = self._full_cond(row)
			self.db.update(self.T_UATTR, {'flags': nf}, cnd)

	def purge(self):
		self.db.delete(self.T_UATTR, CND_DELETED)

	def enable(self, uid=None, force=False):
		udb = Uri(self.dburi)
		udb.enable(uid=uid, force=force)
		del(udb)
		cdb = Cred(self.dburi)
		cdb.enable(uid=uid, force=force)
		del(cdb)
		return self.change(uid, flags='-d', force=force)

	def disable(self, uid=None, force=False):
		udb = Uri(self.dburi)
		udb.disable(uid=uid, force=force)
		del(udb)
		cdb = Cred(self.dburi)
		cdb.disable(uid=uid, force=force)
		del(cdb)
		return self.change(uid, flags='+d', force=force)
