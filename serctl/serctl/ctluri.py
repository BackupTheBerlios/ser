#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctluri.py,v 1.12 2006/04/18 10:43:12 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.dbany   import DBany
from serctl.error   import Error, EDUPL, ENOCOL, EMULTICANON, \
                           ENODOMAIN, ENOUSER, ENOREC, ENOCANON, ENOALIAS
from serctl.flag    import parse_flags, new_flags, clear_canonical, set_canonical, \
                           is_canonical, set_deleted, flag_syms, CND_NO_DELETED, \
                           CND_DELETED, CND_CANONICAL, LOAD_SER, FOR_SERWEB, IS_TO, \
                           IS_FROM, flag_hex, CND_NO_CANONICAL
from serctl.uri     import split_sip_uri
from serctl.utils   import show_opts, tabprint, arg_pairs, idx_dict, no_all
import serctl.ctlhelp

def help(*tmp):
	print """\
Usage:
        ser_uri [options...] [--] [command] [param...]

%s

Commands & parameters:
	ser_uri add       <uri> <uid> [did]
	ser_uri canonical [[[uri] uid] did]
	ser_uri change    [[[uri] uid] did] [-F flags]
	ser_uri disable   [[[uri] uid] did]
	ser_uri enable    [[[uri] uid] did]
	ser_uri rm        [[[uri] uid] did]
	ser_uri purge
	ser_uri show      [[[uri] uid] did]
""" % serctl.ctlhelp.options()

def show(uri=None, uid=None, did=None, **opts):
	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	u = Uri(opts['DB_URI'])
	uri_list, desc = u.show(uri, uid, did, cols=cols, raw=numeric, \
	  limit=limit)

	tabprint(uri_list, desc, rsep, lsep, astab)

def add(uri, uid, did=None, **opts):
	force = opts['FORCE']
	flags = opts['FLAGS']

	u = Uri(opts['DB_URI'])
	u.add(uri, uid, did, flags=flags, force=force)

def change(uri=None, uid=None, did=None, **opts):
	return _change(uri, uid, did, opts)

def _change(uri, uid, did, opts):
	no_all(opts, uri, uid, did)

	force = opts['FORCE']
	flags = opts['FLAGS']

	u = Uri(opts['DB_URI'])
	u.change(uri, uid, did, flags=flags, force=force)

def rm(uri=None, uid=None, did=None, **opts):
	no_all(opts, uri, uid, did)

	force = opts['FORCE']

	u = Uri(opts['DB_URI'])
	u.rm(uri, uid, did, force=force)

def enable(uri=None, uid=None, did=None, **opts):
	no_all(opts, uri, uid, did)

	force = opts['FORCE']

	u = Uri(opts['DB_URI'])
	u.enable(uri, uid, did, force=force)

def disable(uri=None, uid=None, did=None, **opts):
	no_all(opts, uri, uid, did)

	force = opts['FORCE']

	u = Uri(opts['DB_URI'])
	u.disable(uri, uid, did, force=force)

def canonical(uri=None, uid=None, did=None, **opts):
	opts['flags'] = '+c'
	return _change(uri, uid, did, opts)

def purge(**opts):
	u = Uri(opts['DB_URI'])
	u.purge()

class Uri:
	T_URI = 'uri'
	C_URI = ('uid', 'did', 'username', 'flags')
	I_URI = idx_dict(C_URI)
	F_URI = I_URI['flags']

	T_DOM   = 'domain'
	T_UATTR = 'user_attrs'

	def __init__(self, dburi):
		self.dburi = dburi
		self.db = DBany(dburi)

	def _col_idx(self, cols):
		idx = []
		for col in cols:
			try:
				i = self.I_URI[col]
			except KeyError:
				raise Error (ENOCOL, col)
			idx.append(i)
		return tuple(idx)

	def _full_cond(self, row):
		cnd = ['and', CND_NO_DELETED]
		for i in range(len(self.C_URI)):
			cnd.append(('=', self.C_URI[i], row[i]))
		return tuple(cnd)

	def _cond(self, username=None,  did=None, uid=None, all=False, canonical=False):
                cnd =  ['and', 1]
                if not all:
                        cnd.append(CND_NO_DELETED)
		if canonical:
			cnd.append(CND_CANONICAL)
		err = []
		if username is not None:
			cnd.append(('=', 'username', username))
			err.append('username=' + username)
		if did is not None:
			cnd.append(('=', 'did', did))
			err.append('did=' + did)
		if uid is not None:
			cnd.append(('=', 'uid', uid))
			err.append('uid=' + uid)
		err = ' '.join(err)
		return (cnd, err)

	def _get_did(self, domain):
		if domain is None:
			return None
		cnd = ('and', ('=', 'domain', domain), CND_NO_DELETED)
		rows = self.db.select(self.T_DOM, 'did', cnd, limit=1)
		if not rows:
			return None
		return rows[0][0]

	def _get_uids(self, username, did, uid):
		cnd, err = self._cond(username, did, uid)
		rows = self.db.select(self.T_URI, 'uid', cnd)
		uids = {}
		for row in rows:
			uids[row[0]] = 0
		return uids.keys()

	def show(self, uri=None, uid=None, did=None, cols=None, raw=False, limit=0, canonical=False):
		if not cols:
			cols = self.C_URI
		cidx = self._col_idx(cols)
		
		username, domain = split_sip_uri(uri)
		if did is None:
			did = self._get_did(domain)

		cnd, err = self._cond(username, did, uid, all=True, canonical=canonical)
		rows = self.db.select(self.T_URI, self.C_URI, cnd, limit)
		new_rows = []
                for row in rows:
			if not raw:
				row[self.F_URI] = flag_syms(row[self.F_URI])
			new_row = []
			for i in cidx:
				new_row.append(row[i])
			new_rows.append(new_row)
		desc = self.db.describe(self.T_URI)
		desc = [ desc[i] for i in cols ]
		return new_rows, desc

	def default_flags(self):
		return str(LOAD_SER | FOR_SERWEB | IS_TO | IS_FROM)

	def add(self, uri, uid, did=None, flags=None, force=False):
		dflags = self.default_flags()
		fmask  = parse_flags(flags)
		flags  = new_flags(dflags, fmask)
		canonical = is_canonical(flags)

		username, domain = split_sip_uri(uri)
		if did is None:
			did = self._get_did(domain)
		if did is None:
			if force:
				return
			raise Error (ENODOMAIN, domain)

		# exist uid?
		cnd = ('and', ('=', 'uid', uid), \
		  ('=', 'name', 'datetime_created'), CND_NO_DELETED)
		rows = self.db.select(self.T_UATTR, 'uid', cnd, limit=1)
		if not rows and not force:
			raise Error (ENOUSER, uid)

		# uri (if exist & force --> delete, else error)
		cnd, err = self._cond(username, did, uid)
		rows = self.db.select(self.T_URI, 'flags', cnd, limit=1)
		if rows:
			if not force:
				raise Error (EDUPL, err)
			self.rm(uri, uid, did, force=True)

		# update canonical flag
		cnd = ('and', ('=', 'uid', uid), CND_NO_DELETED)
		rows = self.db.select(self.T_URI, self.C_URI, cnd)
		canon_exist = False
		for row in rows:
			if not is_canonical(row[self.F_URI]):
				continue
			if not canonical:
				canon_exist = True
				break
			f = clear_canonical(row[self.F_URI])
			cnd = self._full_cond(row)
			self.db.update(self.T_URI, {'flags':f}, cnd)
		if not canonical and not canon_exist:
			flags = set_canonical(flags)

		# add new URI
		ins = { 'uid' : uid, 'did' : did, 'username' : username, \
			'flags' : flags }
		self.db.insert(self.T_URI, ins)

	def change(self, uri=None, uid=None, did=None, flags=None, \
	  force=False):
		fmask = parse_flags(flags)
		nflags = new_flags(0, fmask)
		canonical = is_canonical(nflags)

		username, domain = split_sip_uri(uri)
		if did is None:
			did = self._get_did(domain)
		if (did is None) and (uri is not None):
			if force:
				return
			raise Error (ENODOMAIN, domain)

		# exist ?
		cond, error = self._cond(username, did, uid)
		rows = self.db.select(self.T_URI, 'flags', cond, limit=2)
		if not rows:
			if force:
				return
			raise Error (ENOUID, err)
		if canonical and (len(rows) > 1) and not force:
			raise Error (EMULTICANON,)

		# clear canon
		if canonical:
			cnd, err = self._cond(uid=uid)
			cnd.append(CND_CANONICAL)
			rows = self.db.select(self.T_URI, self.C_URI, cnd)
			for row in rows:
				nf = clear_canonical(row[self.F_URI])
				cnd = self._full_cond(row)
				self.db.update(self.T_URI, {'flags':nf}, cnd)

		# update flags
		rows = self.db.select(self.T_URI, self.C_URI, cond)
		if not rows and not force:
			raise Error (ENOREC, error)
		for row in rows:
			nf = new_flags(row[self.F_URI], fmask)
			cnd = self._full_cond(row)
			self.db.update(self.T_URI, {'flags':nf}, cnd)

	def rm(self, uri=None, uid=None, did=None, force=False):
		username, domain = split_sip_uri(uri)
		if did is None:
			did = self._get_did(domain)
		if did is None:
			if force:
				return
			raise Error (ENODOMAIN, domain)

		cond, error = self._cond(username, did, uid)
		uids = self._get_uids(username, did, uid)

		# is canonical?
		canon_deleted = True
		cnd = cond + [CND_CANONICAL, ]
		rows = self.db.select(self.T_URI, 'flags', cnd, limit=1)
		if not rows:
			canon_deleted = False

		# rm 
		rows = self.db.select(self.T_URI, self.C_URI, cond)
		if not rows and not force:
			raise Error (ENOREC, error)
		for row in rows:
			nf = set_deleted(row[self.F_URI])
			cnd = self._full_cond(row)
			self.db.update(self.T_URI, {'flags': nf}, cnd)

		# set new canon flag
		if canon_deleted:
			for uid in uids:
				cnd, err = self._cond(uid=uid)
				rows = self.db.select(self.T_URI, self.C_URI, \
				  cnd, limit=1)
				if rows:
					nf = set_canonical(rows[0][self.F_URI])
					cnd = self._full_cond(rows[0])
					upd = {'flags': nf}
					self.db.update(self.T_URI, upd, cnd)

	def rm_user(self, username, quiet=False, purge=False):

		# rm username
		cond, error = self._cond(uid=username)
		rows = self.db.select(self.T_URI, self.C_URI, cond)

		if not quiet:
			if not rows and not force:
				raise Error (ENOREC, error)
		for row in rows:
			nf = set_deleted(row[self.F_URI])
			cnd = self._full_cond(row)
			self.db.update(self.T_URI, {'flags': nf}, cnd)

		if not purge: return

		# rm uids == username
		cond, error = self._cond(uid=username)
		rows = self.db.select(self.T_URI, self.C_URI, cond)

		for row in rows:
			nf = set_deleted(row[self.F_URI])
			cnd = self._full_cond(row)
			self.db.update(self.T_URI, {'flags': nf}, cnd)

	def purge(self):
		self.db.delete(self.T_URI, CND_DELETED)

	def enable(self, uri=None, uid=None, did=None, force=False):
		return self.change(uri, uid, did, flags='-d', force=force)

	def disable(self, uri=None, uid=None, did=None, force=False):
		return self.change(uri, uid, did, flags='+d', force=force)

	def get_alias_info(self, username, canonical=False, force=False):
		cnd, err = self._cond(username)
		if canonical:
			cnd.append(CND_CANONICAL)
		else:
			cnd.append(CND_NO_CANONICAL)
		rows = self.db.select(self.T_URI, ['uid, ''did'], cnd, limit=1)
		if not rows:
			if force:
				return None
			if canonical:
				raise Error (ENOCANON, err)
			else:
				raise Error (ENOALIAS, err)
		return rows[0]

	def get_uids(self, uri, flags=[], force=False):
		username, domain = split_sip_uri(uri)
		did = self._get_did(domain)
		if did is None:
			if force:
				return None
			raise Error (ENODOMAIN, domain)

		cnd, err = self._cond(username, did)
		if flags is not None:
			for flag in flags:
				if flag < 0:
					flag = ~flag
					cnd.append(('=',  ('&', 'flags', flag), 0))
				else:
					cnd.append(('!=',  ('&', 'flags', flag), 0))
		rows = self.db.select(self.T_URI, 'uid', cnd)
		uids = {}
		for row in rows:
			uids[row[0]] = 0
		return uids.keys()
