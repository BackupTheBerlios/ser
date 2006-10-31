#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlcred.py,v 1.16 2006/10/31 19:40:15 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.dbany   import DBany
from serctl.error   import Error, ENOARG, EINVAL, EDUPL, ENOCOL, EMULTICANON, \
                           ENOREC, ENODOMAIN, ENOUSER, ENOUID, ENOSYS, EDB
from serctl.flag    import parse_flags, new_flags, clear_canonical, set_canonical, \
                           is_canonical, set_deleted, flag_syms, CND_NO_DELETED, \
                           CND_DELETED, CND_CANONICAL, LOAD_SER, FOR_SERWEB, cv_flags
from serctl.utils   import show_opts, tabprint, arg_pairs, idx_dict, no_all, \
                           col_idx, cond, full_cond, get_password, uniq
import md5, serctl.ctlhelp, serctl.ctldomain, serctl.ctluser, serctl.ctlattr


def help(*tmp):
	print """\
Usage:
	ser_cred [options...] [--] [command] [param...]

%s

Commands & parameters:
	ser_cred add      <uid> <auth_username> <realm> [password]
	ser_cred change   <auth_username> <realm> [-p password] [-F flags]
	ser_cred disable  <auth_username> <realm>
	ser_cred enable   <auth_username> <realm>
	ser_cred rm       <auth_username> <realm>
	ser_cred password <auth_username> <realm> [-p password]
	ser_cred purge
	ser_cred show     [realm [auth_username]]
""" % serctl.ctlhelp.options()


def show(realm=None, auth_username=None, **opts):

	cols, fformat, limit, rsep, lsep, astab = show_opts(opts)

	u = Cred(opts['DB_URI'])
	clist, desc = u.show(realm, auth_username, cols, fformat, limit)

	tabprint(clist, desc, rsep, lsep, astab)

def add(uid, auth_username, realm, password=None, **opts):
	password = get_password(opts, password)
	force = opts['FORCE']
	flags = opts['FLAGS']

	u = Cred(opts['DB_URI'])
	u.add(uid, auth_username, realm, password, flags, force)

def rm(auth_username, realm, **opts):
	force = opts['FORCE']

	u = Cred(opts['DB_URI'])
	u.rm(auth_username, realm, force=force)


def change(auth_username, realm, password=None, **opts):
	password = opts.get('PASSWORD', password)

	force = opts['FORCE']
	flags = opts['FLAGS']

	u = Cred(opts['DB_URI'])
	u.change(auth_username, realm, password, flags, force)

def password(auth_username, realm, password=None, **opts):
	password = get_password(opts, password)

	force = opts['FORCE']
	flags = opts['FLAGS']

	u = Cred(opts['DB_URI'])
	u.change(auth_username, realm, password, flags, force)

def _change_flags(auth_username, realm, flags, opts):
	force = opts['FORCE']
	u = Cred(opts['DB_URI'])
	u.change(auth_username, realm, None, flags, force)

def enable(auth_username, realm, **opts):
	_change_flags(auth_username, realm, '-d', opts)

def disable(auth_username, realm, **opts):
	_change_flags(auth_username, realm, '+d', opts)


def purge(**opts):
	u = Cred(opts['DB_URI'])
	u.purge()

class Cred:
	TABLE = 'credentials'
	COLUMNS = ('auth_username', 'realm', 'uid', 'password', 'ha1', \
	           'ha1b', 'flags')
	COLIDXS = idx_dict(COLUMNS)
	FLAGIDX = COLIDXS['flags']

	def __init__(self, dburi, db=None):
		self.Domain_attrs = serctl.ctlattr.Domain_attrs
		self.Domain = serctl.ctldomain.Domain
		self.User = serctl.ctluser.User
		self.dburi = dburi
		if db is not None:
			self.db = db
		else:
			self.db = DBany(dburi)

	def default_flags(self):
		return str(LOAD_SER | FOR_SERWEB)

	def get_uid(self, username, realm):
		cnd, err = cond(CND_NO_DELETED, auth_username=username, realm=realm)
		rows = self.db.select(self.TABLE, 'uid', cnd)
		if not rows:
			raise Error (ENOREC, err)
		uids = uniq([ i[0] for i in rows ])
		if len(uids) > 1:
			raise Error (EDB, '%s@%s=%s' % (username, realm, str(uids)))
		uid = uids[0]
		return uid

	def get_uids_for_username(self, username):
		cnd, err = cond(CND_NO_DELETED, auth_username=username)
		rows = self.db.select(self.TABLE, 'uid', cnd)
		if not rows:
			raise Error (ENOREC, err)
		uids = [ i[0] for i in rows ]
		return uniq(uids)

	def exist(self, username, realm):
		cnd, err = cond(CND_NO_DELETED, auth_username=username, realm=realm)
		rows = self.db.select(self.TABLE, 'uid', cnd, limit=1)
		return rows != []

	def exist_uid(self, uid):
		cnd, err = cond(CND_NO_DELETED, uid=uid)
		rows = self.db.select(self.TABLE, 'uid', cnd, limit=1)
		return rows != []

	def exist_realm(self, realm):
		cnd, err = cond(CND_NO_DELETED, realm=realm)
		rows = self.db.select(self.TABLE, 'realm', cnd, limit=1)
		return rows != []

	def _show(self, cnd, err, cols, fformat, limit):
		if not cols:
			cols = self.COLUMNS
		cidx = col_idx(self.COLIDXS, cols)

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

	def show(self,  realm=None, username=None, cols=None, fformat='raw', limit=0):
		cnd, err = cond(auth_username=username, realm=realm)
		return self._show(cnd, err, cols, fformat, limit)

	def show_uid(self, uid, cols=None, fformat='raw', limit=0):
		cnd, err = cond(uid=uid)
		return self._show(cnd, err, cols, fformat, limit)

	def hashes(self, username, realm, password):
		uri = '@'.join((username, realm))
		hash_src1  = ':'.join((username, realm, password))
		hash_src1b = ':'.join((uri, realm, password))
		ha1  = md5.new(hash_src1).hexdigest()
		ha1b = md5.new(hash_src1b).hexdigest()
		return (ha1, ha1b)


	def add(self, uid, username, realm, password, flags=None, force=False):
		dflags = self.default_flags()
		fmask  = parse_flags(flags)
		flags  = new_flags(dflags, fmask)

		do = self.Domain(self.dburi, self.db)
		try:
			did = do.get_did(realm)
		except:
			if force: return
			raise

		us = self.User(self.dburi, self.db)
		if not us.exist(uid) and not force:
			raise Error (ENOUSER, 'uid='+uid)

		if self.exist(username, realm):
			if force: return
			raise Error (EDUPL, )

		# compute hashes
		ha1, ha1b = self.hashes(username, realm, password)

		# add new cred
		ins = { 'uid' : uid, 'auth_username' : username, \
			'realm' : realm, 'password' : password, \
			'ha1' : ha1, 'ha1b' : ha1b, 'flags' : flags \
		}
		self.db.insert(self.TABLE, ins)

	def _rm(self, cnd, err, force):
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		if not rows and not force:
			raise Error (ENOREC, err)
		for row in rows:
			nf = set_deleted(row[self.FLAGIDX])
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags': nf}, cnd)

	def rm(self, username, realm, force=False):
		try:
			uid = self.get_uid(username, realm)
		except:
			if force: return
			raise
		cnd, err = cond(CND_NO_DELETED, uid=uid, realm=realm)
		return self._rm(cnd, err, force)

	def rm_realm(self, realm, force=False):
		cnd, err = cond(CND_NO_DELETED, realm=realm)
		return self._rm(cnd, err, force)

	def rm_uid(self, uid, force=False):
		cnd, err = cond(CND_NO_DELETED, uid=uid)
		return self._rm(cnd, err, force)

	def change(self, username, realm, password=None, flags=None, force=False):
		upd = {}
		# username & realm is required for password change
		if password is not None:
			# compute hashes
			ha1, ha1b = self.hashes(username, realm, password)
			upd = {'ha1': ha1, 'ha1b': ha1b, 'password': password}

		fmask = parse_flags(flags)

		# update
		cnd, err = cond(CND_NO_DELETED, auth_username=username, realm=realm)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		if not rows and not force:
			raise Error (ENOREC, err)
		for row in rows:
			cnd = full_cond(self.COLUMNS, row)
			if flags is not None:
				nf = new_flags(row[self.FLAGIDX], fmask)
				upd['flags'] = nf
			self.db.update(self.TABLE, upd, cnd)

	def purge(self):
		self.db.delete(self.TABLE, CND_DELETED)
