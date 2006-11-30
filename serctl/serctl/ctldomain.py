#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# Copyright (C) 2005 FhG Fokus
#
# This is part of Ser (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.dbany   import DBany
from serctl.error   import Error, ENOARG, EINVAL, EDUPL, ENOCOL, EDOMAIN, ENOREC, \
                           EMULTICANON, ENODOMAIN, ENODID
from serctl.flag    import parse_flags, new_flags, clear_canonical, set_canonical, \
                           is_canonical, set_deleted, flag_syms, CND_NO_DELETED, \
                           CND_DELETED, CND_CANONICAL, LOAD_SER, FOR_SERWEB, \
                           cv_flags
from serctl.utils   import show_opts, tabprint, arg_pairs, idx_dict, no_all, \
                           col_idx, cond, full_cond, errstr
import serctl.ctlhelp, serctl.ctlcred, serctl.ctluri, serctl.ctlattr

def help(*tmp):
	print """\
Usage:
	ser_domain [options...] [--] [command] [param...]

%s

Commands & parameters:
	ser_domain add       <did> <domain> [domain_alias...]
	ser_domain canonical [domain]
	ser_domain change    [domain] [-F flags]
	ser_domain disable   [domain]
	ser_domain enable    [domain]
	ser_domain rm        [domain]
	ser_domain purge
	ser_domain show      [domain]
""" % serctl.ctlhelp.options()


def show(domain=None, **opts):
	cols, fformat, limit, rsep, lsep, astab = show_opts(opts)

	u = Domain(opts['DB_URI'])

	if opts['DID']:
		uri_list, desc = u.show_did(domain, cols=cols, fformat=fformat, limit=limit)
	elif opts['DEPTH']:
		uri_list, desc = u.show_did_for_domain(domain, cols=cols, fformat=fformat, limit=limit)
	else:
		uri_list, desc = u.show_domain(domain, cols=cols, fformat=fformat, limit=limit)

	tabprint(uri_list, desc, rsep, lsep, astab)

def add(did, *domains, **opts):
	flags = opts['FLAGS']
	force = opts['FORCE']
	if not domains:
		raise Error (ENOARG, 'domain')

	u = Domain(opts['DB_URI'])

	if not force:
		for dom in domains:
			if u.exist_domain(dom):
				raise Error (EDUPL, 'domain=' + str(dom))

	for dom in domains:
		u.add(did, dom, flags, force)

def rm(domain=None, **opts):
	no_all(opts, domain)
	force = opts['FORCE']

	u = Domain(opts['DB_URI'])

	if opts['DID']:
		u.rm_did(domain, force)
	elif opts['DEPTH']:
		u.rm_did_for_domain(domain, force)
	else:
		u.rm_domain(domain, force)

def _change(domain, opts):
	no_all(opts, domain)

	force = opts['FORCE']
	flags = opts['FLAGS']

	u = Domain(opts['DB_URI'])
	u.change_domain(domain, flags=flags, force=force)

def change(domain=None, **opts):
	return _change(domain, opts)

def enable(domain=None, **opts):
	opts['FLAGS'] = '-d'
	return _change(domain, opts)

def disable(domain=None, **opts):
	opts['FLAGS'] = '+d'
	return _change(domain, opts)

def canonical(domain, **opts):
	opts['FLAGS'] = '+c'
	return _change(domain, opts)

def purge(**opts):
	u = Domain(opts['DB_URI'])
	u.purge()
	u = serctl.ctlattr.Domain_attrs(opts['DB_URI'])
	u.purge()

class Domain:
	TABLE = 'domain'
	COLUMNS = ('did', 'domain', 'flags')
	COLIDXS = idx_dict(COLUMNS) # column index dict
	FLAGIDX = COLIDXS['flags']  # flags column index 

	def __init__(self, dburi, db=None):
		self.Uri   = serctl.ctluri.Uri
		self.Cred  = serctl.ctlcred.Cred
		self.Domain_attrs  = serctl.ctlattr.Domain_attrs
		self.dburi = dburi
		if db is not None:
			self.db = db
		else:
			self.db = DBany(dburi)

	def default_flags(self):
		return str(LOAD_SER | FOR_SERWEB)

	def get_did(self, domain):
		cnd, err = cond(CND_NO_DELETED, domain=domain)
		rows = self.db.select(self.TABLE, 'did', cnd, limit=1)
		if not rows:
			raise Error (ENOREC, err)
		return rows[0][0]

	def get_domains(self, did):
		cnd, err = cond(CND_NO_DELETED, did=did)
		rows = self.db.select(self.TABLE, 'domain', cnd)
		if not rows:
			raise Error (ENOREC, err)
		return [ row[0] for row in rows ]

	def get_domain(self, did):
		cnd, err = cond(CND_NO_DELETED, CND_CANONICAL, did=did)
		rows = self.db.select(self.TABLE, 'domain', cnd, limit=1)
		if not rows:
			cnd, err = cond(CND_NO_DELETED, did=did)
			rows = self.db.select(self.TABLE, 'domain', cnd, limit=1)
		if not rows:
			raise Error (ENOREC, err)
		return rows[0][0]

	def exist(self, did, domain):
		cnd, err = cond(CND_NO_DELETED, did=did, domain=domain)
		rows = self.db.select(self.TABLE, 'did', cnd, limit=1)
		return rows != []

	def exist_domain(self, domain):
		cnd, err = cond(CND_NO_DELETED, domain=domain)
		rows = self.db.select(self.TABLE, 'domain', cnd, limit=1)
		return rows != []

	def exist_did(self, did):
		cnd, err = cond(CND_NO_DELETED, did=did)
		rows = self.db.select(self.TABLE, 'did', cnd, limit=1)
		return rows != []

	def is_last_domain(self, did, domain):
		cnd, err = cond(CND_NO_DELETED, did=did)
		cnd.append(('!=', 'domain', domain))
		rows = self.db.select(self.TABLE, 'did', cnd, limit=1)
		return rows == []

	def is_used(self, did):
		ur = self.Uri(self.dburi, self.db)
		cr = self.Cred(self.dburi, self.db)
		return ur.exist_did(did) or cr.exist_realm(did)

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

	def show_domain(self, domain=None, cols=None, fformat='raw', limit=0):
		cnd, err = cond(domain=domain)
		rows, desc = self._show(cnd, err, cols, fformat, limit)
		if domain and not rows:
			raise Error(ENODOMAIN, domain)
		return rows, desc

	def show_did(self, did=None, cols=None, fformat='raw', limit=0):
		cnd, err = cond(did=did)
		rows, desc = self._show(cnd, err, cols, fformat, limit)
		if did and not rows:
			raise Error(ENODID, did)
		return rows, desc

	def show_did_for_domain(self, domain=None, cols=None, fformat='raw', limit=0):
		if domain is None:
			return self.show_did(None, cols, fformat, limit)
		try:
			did = self.get_did(domain)
		except:
			if not cols:
				cols = self.COLUMNS
			desc = self.db.describe(self.TABLE)
			desc = [ desc[i] for i in cols ]
			return [], desc
		rows, desc = self.show_did(did, cols, fformat, limit)
		return rows, desc

	def add(self, did, domain, flags=None, force=False):
		dflags = self.default_flags()
		fmask  = parse_flags(flags)
		flags  = new_flags(dflags, fmask)
		canonical = is_canonical(flags)

		if self.exist(did, domain):
			if force: return
			raise Error (EDUPL, errstr(did=did, domain=domain))

		# set digest realm attr
		da = self.Domain_attrs(self.dburi, self.db)
		da.set_default(did, 'digest_realm', domain)

		# update canonical flag
		cnd, err = cond(CND_NO_DELETED, did=did)
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

		# add new domain
		ins = { 'did' : did, 'domain' : domain, 'flags' : flags }
		self.db.insert(self.TABLE, ins)

	def rm(self, did, domain, force=False):
		if self.is_last_domain(did, domain):
			return self.rm_did(did, force)

		# is canonical?
		canon_deleted = True
		cnd, err = cond(CND_NO_DELETED, CND_CANONICAL, did=did, domain=domain)
		rows = self.db.select(self.TABLE, 'did', cnd, limit=1)
		if not rows:
			canon_deleted = False

		# remove
		cnd, err = cond(CND_NO_DELETED, did=did, domain=domain)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		if not rows:
			if force: return
			raise Error (ENOREC, err)
		for row in rows:
			nf = set_deleted(row[self.FLAGIDX])
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags': nf}, cnd)

		# set new canon flag
		if canon_deleted:
			cnd, err = cond(CND_NO_DELETED, did=did)
			rows = self.db.select(self.TABLE, self.COLUMNS, cnd, limit=1)
			if rows:
				nf = set_canonical(rows[0][self.FLAGIDX])
				cnd = full_cond(self.COLUMNS, rows[0])
				upd = {'flags': nf}
				self.db.update(self.TABLE, upd, cnd)

	def rm_did(self, did, force=False):
		da = self.Domain_attrs(self.dburi, self.db)

		if self.is_used(did):
			if force:
				self._try_rm_orphans(did)
			else:
				raise Error (EDOMAIN, 'did=%s' % did)

		da.rm_exist(did, 'digest_realm')

		# remove did
		cnd, err = cond(CND_NO_DELETED, did=did)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		if not rows:
			if force: return
			raise Error (ENOREC, err)
		for row in rows:
			nf = set_deleted(row[self.FLAGIDX])
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags': nf}, cnd)

	def rm_domain(self, domain, force=False):
		try:
			did = self.get_did(domain)
		except:
			if force: return
			raise
		self.rm(did, domain, force)

	def rm_did_for_domain(self, domain, force):
		try:
			did = self.get_did(domain)
		except:
			if force: return
			raise
		if self.is_used(did) and not force:
			raise Error (EDOMAIN, errstr(did=did))
		self.rm_did(did, force)

	def _try_rm_orphans(self, did):
		ur = self.Uri(self.dburi, self.db)
		cr = self.Cred(self.dburi, self.db)
		try:
			ur.rm_did(did)
		except:
			pass
		try:
			cr.rm_realm(did)
		except:
			pass

	def change_domain(self, domain, flags=None, force=False):
		upd = {}
		fmask = parse_flags(flags)
		nflags = new_flags(0, fmask)
		canonical = is_canonical(nflags)

		# get did
		try:
			did = self.get_did(domain)
		except:
			if force: return
			raise

		# clear canon
		if canonical:
			cnd, err = cond(CND_NO_DELETED, did=did)
			cnd.append(CND_CANONICAL)
			rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
			for row in rows:
				nf = clear_canonical(row[self.FLAGIDX])
				cnd = full_cond(self.COLUMNS, row)
				upd = {'flags':nf}
				self.db.update(self.TABLE, upd, cnd)

		# update flags
		cnd, err = cond(CND_NO_DELETED, domain=domain)
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd)
		if not rows and not force:
			raise Error (ENOREC, err)
		for row in rows:
			nf = new_flags(row[self.FLAGIDX], fmask)
			cnd = full_cond(self.COLUMNS, row)
			self.db.update(self.TABLE, {'flags':nf}, cnd)


	def purge(self):
		self.db.delete(self.TABLE, CND_DELETED)

