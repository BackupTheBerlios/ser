#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlcred.py,v 1.2 2006/01/06 14:59:43 hallik Exp $
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

from dbany   import DBany
from error   import Error, ENOARG, EINVAL, EDUPL, ENOCOL, EMULTICANON, \
                    ENOREC, ENODOMAIN, ENOUSER, ENOUID, ENOSYS, EUMAP
from flag    import parse_flags, new_flags, clear_canonical, set_canonical, \
                    is_canonical, set_deleted, flag_syms, CND_NO_DELETED, \
                    CND_DELETED, CND_CANONICAL, LOAD_SER, FOR_SERWEB
from options import CMD_ADD, CMD_CANONICAL, CMD_DISABLE, CMD_ENABLE, CMD_HELP, \
                    CMD_CHANGE, CMD_RM, CMD_SHOW, CMD_PURGE, OPT_PASSWORD, \
                    OPT_DATABASE, OPT_FORCE, OPT_LIMIT, OPT_FLAGS
from utils   import show_opts, tabprint, arg_pairs, idx_dict, no_all
import md5

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
	ser_cred [options...] [--] [command] [param...]

Commands & parameters:
	ser_cred add     <auth_username> <realm> <uid> <password>
	ser_cred change  [[[auth_username] realm] uid] [-p password] [-F flags]
	ser_cred disable [[[auth_username] realm] uid]
	ser_cred enable  [[[auth_username] realm] uid]
	ser_cred rm      [[[auth_username] realm] uid]
	ser_cred purge
	ser_cred show    [[[auth_username] realm] uid]
"""

def _get_username(args, mandatory=True):
	try:
		username = args[0]
	except:
		if mandatory:
			raise Error (ENOARG, 'auth_username')
		else:
			username = None
	return username

def _get_realm(args, mandatory=True):
	try:
		realm = args[1]
	except:
		if mandatory:
			raise Error (ENOARG, 'realm')
		else:
			realm = None
	return realm

def _get_uid(args, mandatory=True):
	try:
		uid = args[2]
	except:
		if mandatory:
			raise Error (ENOARG, 'uid')
		else:
			uid = None
	return uid

def _get_pass(args, mandatory=True):
	try:
		password = args[3]
	except:
		if mandatory:
			raise Error (ENOARG, 'password')
		else:
			password = None
	return password

def show(db, args, opts):
	username = _get_username(args, False)
	realm    = _get_realm(args, False)
	uid      = _get_uid(args, False)

	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	u = Cred(db)
	cred_list, desc = u.show(username, realm, uid, cols=cols, \
	  raw=numeric, limit=limit)

	tabprint(cred_list, desc, rsep, lsep, astab)

def add(db, args, opts):
	username = _get_username(args)
	realm    = _get_realm(args)
	uid      = _get_uid(args)
	password = opts.get(OPT_PASSWORD)
	if password is None:
		password = _get_pass(args)

	force    = opts.has_key(OPT_FORCE)
	flags    = opts.get(OPT_FLAGS)

	u = Cred(db)
	u.add(username, realm, uid, password, flags=flags, force=force)

def change(db, args, opts):
	username = _get_username(args, False)
	realm    = _get_realm(args, False)
	uid      = _get_uid(args, False)
	password = _get_pass(args, False)

	no_all(opts, username, realm, uid)

	force    = opts.has_key(OPT_FORCE)
	flags    = opts.get(OPT_FLAGS)
	password = opts.get(OPT_PASSWORD, password)

        u = Cred(db)
        u.change(username, realm, uid, password, flags=flags, force=force)

def rm(db, args, opts):
	username = _get_username(args, False)
	realm    = _get_realm(args, False)
	uid      = _get_uid(args, False)

	no_all(opts, username, realm, uid)

	force = opts.has_key(OPT_FORCE)

	u = Cred(db)
	u.rm(username, realm, uid, force=force)

def enable(db, args, opts):
	username = _get_username(args, False)
	realm    = _get_realm(args, False)
	uid      = _get_uid(args, False)

	no_all(opts, username, realm, uid)

	force    = opts.has_key(OPT_FORCE)

        u = Cred(db)
        u.enable(username, realm, uid, force=force)

def disable(db, args, opts):
	username = _get_username(args, False)
	realm    = _get_realm(args, False)
	uid      = _get_uid(args, False)

	no_all(opts, username, realm, uid)

	force    = opts.has_key(OPT_FORCE)

        u = Cred(db)
        u.disable(username, realm, uid, force=force)

def purge(db, args, opts):
	u = Cred(db)
	u.purge()

class Cred:
	T_CRED = 'credentials'
	C_CRED = ('auth_username', 'realm', 'uid', 'password', 'ha1', \
	  'ha1b', 'flags')
	I_CRED = idx_dict(C_CRED)
	F_CRED = I_CRED['flags']

	T_DATTR = 'domain_attrs'
	T_UATTR = 'user_attrs'

	def __init__(self, dburi):
		self.dburi = dburi
		self.db = DBany(dburi)

	def _col_idx(self, cols):
		idx = []
		for col in cols:
			try:
				i = self.I_CRED[col]
			except KeyError:
				raise Error (ENOCOL, col)
			idx.append(i)
		return tuple(idx)

	def _full_cond(self, row):
		cnd = ['and', CND_NO_DELETED]
		for i in range(len(self.C_CRED)):
			cnd.append(('=', self.C_CRED[i], row[i]))
		return tuple(cnd)

	def _cond(self, username=None, realm=None, uid=None, all=False):
		cnd =  ['and']
		if not all:
			cnd.append(CND_NO_DELETED)
		err = []
		if username is not None:
			cnd.append(('=', 'auth_username', username))
			err.append('auth_username=' + username)
		if realm is not None:
			cnd.append(('=', 'realm', realm))
			err.append('realm=' + realm)
		if uid is not None:
			cnd.append(('=', 'uid', uid))
			err.append('uid=' + uid)
		err = ' '.join(err)
		return (cnd, err)

	def show(self, username=None, realm=None, uid=None, cols=None, \
	  raw=False, limit=0):
		if not cols:
			cols = self.C_CRED
		cidx = self._col_idx(cols)

		cnd, err = self._cond(username, realm, uid, all=True)
		if len(cnd) < 3:
			cnd = None
		rows = self.db.select(self.T_CRED, self.C_CRED, cnd, limit)
		new_rows = []
		for row in rows:
			if not raw:
				row[self.F_CRED] = flag_syms(row[self.F_CRED])
 			new_row = []
			for i in cidx:
				new_row.append(row[i])
			new_rows.append(new_row)
		desc = self.db.describe(self.T_CRED)
		desc = [ desc[i] for i in cols ]
		return new_rows, desc

	def default_flags(self):
		return str(LOAD_SER | FOR_SERWEB)

	def hashes(self, username, realm, password):
		uri = '@'.join((username, realm))
		hash_src1  = ':'.join((username, realm, password))
		hash_src1b = ':'.join((uri, realm, password))
		ha1  = md5.new(hash_src1).hexdigest()
		ha1b = md5.new(hash_src1b).hexdigest()
		return (ha1, ha1b)

	def _chk_realm_in_dattr(self, realm):
		cnd = ('and', ('=', 'name', 'digest_realm'), \
		  ('=', 'value', realm), CND_NO_DELETED)
		rows = self.db.select(self.T_DATTR, 'did', cnd, limit=1)
		if not rows:
			raise Error (ENODOMAIN, realm)

	def _chk_uid_in_uattr(self, uid):
		cnd = ('and', ('=', 'uid', uid), CND_NO_DELETED, \
		  ('=', 'name', 'datetime_created'))
		rows = self.db.select(self.T_UATTR, 'uid', cnd, limit=1)
		if not rows:
			raise Error (ENOUSER, uid)

	def _chk_uid_map(self, username, realm, uid):
		cnd = ('and', ('=', 'realm', realm), CND_NO_DELETED, \
		  ('=', 'auth_username', username))
		rows = self.db.select(self.T_CRED, 'uid', cnd)
		for row in rows:
			if row[0] != uid:
				raise Error (EUMAP,)

	def add(self, username, realm, uid, password, flags=None, \
	  force=False):
		dflags = self.default_flags()
		fmask  = parse_flags(flags)
		flags  = new_flags(dflags, fmask)

		# exist realm and uid? 
		if not force:
			self._chk_realm_in_dattr(realm)
			self._chk_uid_in_uattr(uid)
			self._chk_uid_map(username, realm, uid)

		# cred (if exist & force --> delete, else error)
		cnd, err = self._cond(username, realm, uid)
		rows = self.db.select(self.T_CRED, 'flags', cnd, limit=1)
		if rows:
			if not force:
				raise Error (EDUPL, err)
			self.rm(username, realm, uid, force=True)

		# compute hashes
		ha1, ha1b = self.hashes(username, realm, password)

		# add new cred
		ins = { 'uid' : uid, 'auth_username' : username, \
			'realm' : realm, 'password' : password, \
			'ha1' : ha1, 'ha1b' : ha1b, 'flags' : flags \
		}
		self.db.insert(self.T_CRED, ins)

	def change(self, username=None, realm=None, uid=None, password=None, \
	  flags=None, force=False):
		upd = {}
		# username & realm is required for password change
		if password is not None:
			if username is None:
				raise Error (ENOARG, 'auth_username')
			if realm is None:
				raise Error (ENOARG, 'realm')
			# compute hashes
			ha1, ha1b = self.hashes(username, realm, password)
			upd = {'ha1': ha1, 'ha1b': ha1b, 'password': password}

		# exist realm and uid?
		if not force:
			if realm is not None:
				self._chk_realm_in_dattr(realm)
			if uid is not None:
				self._chk_uid_in_uattr(uid)

		fmask = parse_flags(flags)

		# update
		cnd, err = self._cond(username, realm, uid)
		rows = self.db.select(self.T_CRED, self.C_CRED, cnd)
		if not rows and not force:
			raise Error (ENOREC, err)
		for row in rows:
			cnd = self._full_cond(row)
			if flags is not None:
				nf = new_flags(row[self.F_CRED], fmask)
				upd['flags'] = nf
			self.db.update(self.T_CRED, upd, cnd)

	def rm(self, username=None, realm=None, uid=None, force=False):
		cnd, err = self._cond(username, realm, uid)
		rows = self.db.select(self.T_CRED, self.C_CRED, cnd)
		if not rows and not force:
			raise Error (ENOREC, err)
		for row in rows:
			nf = set_deleted(row[self.F_CRED])
			cnd = self._full_cond(row)
			self.db.update(self.T_CRED, {'flags': nf}, cnd)

	def purge(self):
		self.db.delete(self.T_CRED, CND_DELETED)

	def enable(self, username=None, realm=None, uid=None, force=False):
		return self.change(username, realm, uid, flags='-d', force=force)

	def disable(self, username=None, realm=None, uid=None, force=False):
		return self.change(username, realm, uid, flags='+d', force=force)
