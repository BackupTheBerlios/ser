#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctluri.py,v 1.20 2007/10/19 20:27:24 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

#from serctl.ctldomain import Domain
from serctl.dbany     import DBany
from serctl.error     import Error, EDUPL, ENOCOL, EMULTICANON, ENOARG, EDB, \
                             ENODOMAIN, ENOUSER, ENOREC, ENOCANON, ENOALIAS
from serctl.flag      import parse_flags, new_flags, clear_canonical, set_canonical, \
                             is_canonical, set_deleted, flag_syms, CND_NO_DELETED, \
                             CND_DELETED, CND_CANONICAL, LOAD_SER, FOR_SERWEB, IS_TO, \
                             IS_FROM, CND_NO_CANONICAL, cv_flags
from serctl.uri       import split_sip_uri
from serctl.utils     import show_opts, tabprint, arg_pairs, idx_dict, no_all, \
                             col_idx, cond, full_cond, Basectl, CND_TRUE, \
                             CND_FALSE, uniq
import serctl.ctlhelp, serctl.ctldomain


def help(*tmp):
	print """\
Usage:
        ser_uri [options...] [--] [command] [param...]

%s

Commands & parameters:
	ser_uri add       <uid> <uri> [uri...]
	ser_uri canonical <uid> <uri>
	ser_uri change    <uid> <uri> [-F flags]
	ser_uri disable   <uid> <uri> 
	ser_uri enable    <uid> <uri>
	ser_uri purge
	ser_uri rm        <uid> [uri]
	ser_uri show      [uri]
""" % serctl.ctlhelp.options()

def show(uri=None, **opts):
	cols, fformat, limit, rsep, lsep, astab = show_opts(opts)

	u = Uri(opts['DB_URI'])
	uri_list, desc = u.show(uri, cols, fformat, limit)

	tabprint(uri_list, desc, rsep, lsep, astab)

def add(uid, *uris, **opts):
	force = opts['FORCE']
	flags = opts['FLAGS']

	if not uris:
		raise Error (ENOARG, 'uri')

	u = Uri(opts['DB_URI'])

	if not force:
		for uri in uris:
			if u.exist_uid_uri(uid, uri):
				raise Error (EDUPL, uri)
	for uri in uris:
		u.add(uid, uri, flags, force)

def rm(uid, uri=None, **opts):
	force = opts['FORCE']

	u = Uri(opts['DB_URI'])
	if uri is None:
		u.rm_uid(uid, force)
	else:
		u.rm_uid_uri(uid, uri, force)

def _change(uid, uri, opts):
	force = opts['FORCE']
	flags = opts['FLAGS']

	u = Uri(opts['DB_URI'])
	u.change(uid, uri, flags, force)

def change(uid, uri, **opts):
	return _change(uid, uri, opts)

def enable(uid, uri, **opts):
	opts['FLAGS'] = '-d'
	return _change(uid, uri, opts)

def disable(uid, uri, **opts):
	opts['FLAGS'] = '+d'
	return _change(uid, uri, opts)

def canonical(uid, uri, **opts):
	opts['FLAGS'] = '+c'
	return _change(uid, uri, opts)

def purge(**opts):
	u = Uri(opts['DB_URI'])
	u.purge()

class Uri(Basectl):
	TABLE = 'uri'
	COLUMNS = ('uid', 'did', 'username', 'flags', 'scheme')
	COLIDXS = idx_dict(COLUMNS)
	FLAGIDX = COLIDXS['flags']


	def __init__(self, dburi, db=None):
		Basectl.__init__(self, dburi, db)
		self.Domain = serctl.ctldomain.Domain
		self.User   = serctl.ctluser.User

	def default_flags(self):
		return str(LOAD_SER | FOR_SERWEB | IS_TO | IS_FROM)

	def uri2id(self, uri):
		username, domain = split_sip_uri(uri)
		do = self.Domain(self.dburi, self.db)
		did = do.get_did(domain)
		return username, did

	def get_uids(self, uri):
		username, did = self.uri2id(uri)
		cnd, err = cond(CND_NO_DELETED, username=username, did=did)
		rows = self.db.select(self.TABLE, 'uid', cnd)
		if not rows:
			raise Error (ENOREC, err)
		uids = [ i[0] for i in rows ]
		return uniq(uids)

	def get_uid(self, uri):
		uids = self.get_uids(uri)
		if len(uids) > 1:
			raise Error (EDB, '%s=%s' % (uri, str(uids)))
		return uids[0]

	def get_uids_for_username(self, username):
		cnd, err = cond(CND_NO_DELETED, username=username)
		rows = self.db.select(self.TABLE, 'uid', cnd)
		if not rows:
			raise Error (ENOREC, err)
		uids = [ i[0] for i in rows ]
		return uniq(uids)

	def canonize(self, uri):
		domain = split_sip_uri(uri)[1]
		do = self.Domain(self.dburi, self.db)
		did = do.get_did(domain)
		cdomain = do.get_domain(did)

		uid = self.get_uid(uri)
		cnd, err = cond(CND_NO_DELETED, CND_CANONICAL, did=did, uid=uid)
		rows = self.db.select(self.TABLE, 'username', cnd, limit=1)
		if not rows:
			raise Error (ENOREC, err)
		username = rows[0][0]
		return username + '@' + cdomain

	def exist_did(self, did):
		cnd, err = cond(CND_NO_DELETED, did=did)
		rows = self.db.select(self.TABLE, 'did', cnd, limit=1)
		return rows != []

	def exist_uid(self, uid):
		cnd, err = cond(CND_NO_DELETED, uid=uid)
		rows = self.db.select(self.TABLE, 'uid', cnd, limit=1)
		return rows != []

	def exist(self, uid, username, did):
		cnd, err = cond(CND_NO_DELETED, uid=uid, username=username, did=did)
		rows = self.db.select(self.TABLE, 'uid', cnd, limit=1)
		return rows != []

	def exist_uid_uri(self, uid, uri):
		try:
			username, did = self.uri2id(uri)
		except:
			return False
		return self.exist(uid, username, did)

	def exist_username_did(self, username, did):
		cnd, err = cond(CND_NO_DELETED, username=username, did=did)
		rows = self.db.select(self.TABLE, 'uid', cnd, limit=1)
		return rows != []

	def exist_uri(self, uri):
		try:
			username, did = self.uri2id(uri)
		except:
			return False
		return self.exist_username_did(username, did)

	def show(self, uri=None, cols=None, fformat='raw', limit=0):
		if uri:
			try:
				user, did = self.uri2id(uri)
				ce = cond(username=user, did=did)
			except:
				ce = (CND_FALSE, '')
		else:
			ce = (CND_TRUE, '')
		return self.show_cnd(ce, cols, fformat, limit)

	def show_uid(self, uid, cols=None, fformat='raw', limit=0):
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

	def add(self, uid, uri, flags=None, force=False):
		dflags = self.default_flags()
		fmask  = parse_flags(flags)
		flags  = new_flags(dflags, fmask)
		canonical = is_canonical(flags)

		try:
			username, did = self.uri2id(uri)
		except:
			if force: return
			raise

		us = self.User(self.dburi, self.db)
		if not us.exist(uid) and not force:
			raise Error (ENOUSER, uid)

		if self.exist(uid, username, did):
			if force: return
			raise Error (EDUPL, username)

		# update canonical flag
		cnd, err = cond(CND_NO_DELETED, uid=uid)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		canon_exist = False
		for row in rows:
			if not is_canonical(row[self.FLAGIDX]):
				continue
			if not canonical:
				canon_exist = True
				break
			f = clear_canonical(row[self.FLAGIDX])
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags':f}, cnd)
		if not canonical and not canon_exist:
			flags = set_canonical(flags)

		# add new URI
		ins = { 'uid' : uid, 'did' : did, 'username' : username, \
			'flags' : flags }
		self.db.insert(self.TABLE, ins)

	def rm(self, uid, username, did, force=False):
		# is canonical?
		canon_deleted = True
		cnd, err = cond(CND_NO_DELETED, CND_CANONICAL, username=username, did=did, uid=uid)
		rows = self.db.select(self.TABLE, 'flags', cnd, limit=1)
		if not rows:
			canon_deleted = False

		# rm 
		cnd, err = cond(CND_NO_DELETED, username=username, did=did, uid=uid)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		if not rows and not force:
			raise Error (ENOREC, err)
		for row in rows:
			nf = set_deleted(row[self.FLAGIDX])
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags': nf}, cnd)

		# set new canon flag
		if canon_deleted:
			cnd, err = cond(CND_NO_DELETED, uid=uid)
			rows = self.db.select(self.TABLE, self.COLUMNS, cnd, limit=1)
			if rows:
				nf = set_canonical(rows[0][self.FLAGIDX])
				cnd = full_cond(self.COLUMNS, rows[0])
				upd = {'flags': nf}
				self.db.update(self.TABLE, upd, cnd)

	def rm_did(self, did, force=False):
		cnd, err = cond(CND_NO_DELETED, did=did)
		rows = self.db.select(self.TABLE, ['username', 'uid'], cnd)
		if not rows and not force:
			raise Error (ENOREC, err)
		for username, uid in rows:
			self.rm(uid, username, did, force)

	def rm_uid_uri(self, uid, uri, force=False):
		try:
			username, did = self.uri2id(uri)
		except:
			if force: return
			raise

		self.rm(uid, username, did, force)

	def rm_uri(self, uri, force=False):
		try:
			uid = self.get_uid(uri)
		except:
			if force: return
			raise
		self.rm_uid_uri(uid, uri, force)

	def rm_uid(self, uid, force=False):
		cnd, err = cond(CND_NO_DELETED, uid=uid)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		if not rows and not force:
			raise Error (ENOREC, err)
		for row in rows:
			nf = set_deleted(row[self.FLAGIDX])
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags': nf}, cnd)

	def change(self, uid, uri, flags=None, force=False):
		fmask = parse_flags(flags)
		nflags = new_flags(0, fmask)
		canonical = is_canonical(nflags)

		try:
			username, did = self.uri2id(uri)
		except:
			if force: return
			raise
		
		cndt, err = cond(CND_NO_DELETED, uid=uid, username=username, did=did)
		if not self.exist(uid, username, did):
			if force: return
			raise Error (ENOREC, err)

		# clear canon
		if canonical:
			cnd, err = cond(CND_NO_DELETED, uid=uid)
			cnd.append(CND_CANONICAL)
			rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
			for row in rows:
				nf = clear_canonical(row[self.FLAGIDX])
				cnd = full_cond(self.COLUMNS, row)
				self.db.update(self.TABLE, {'flags':nf}, cnd)

		# update flags
		rows = self.db.select(self.TABLE, self.COLUMNS, cndt)
		if not rows and not force:
			raise Error (ENOREC, err)
		for row in rows:
			nf = new_flags(row[self.FLAGIDX], fmask)
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags':nf}, cnd)

	def purge(self):
		self.db.delete(self.TABLE, CND_DELETED)

