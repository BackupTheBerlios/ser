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

from serctl.ctlcred import Cred
from serctl.ctluri  import Uri
from serctl.dbany   import DBany
from serctl.error   import Error, ENOARG, EINVAL, EDUPL, ENOCOL, EDOMAIN, ENOREC, \
                           EMULTICANON, ENODOMAIN
from serctl.flag    import parse_flags, new_flags, clear_canonical, set_canonical, \
                           is_canonical, set_deleted, flag_syms, CND_NO_DELETED, \
                           CND_DELETED, CND_CANONICAL, LOAD_SER, FOR_SERWEB
from serctl.utils   import show_opts, tabprint, arg_pairs, idx_dict, no_all, timestamp
import serctl.ctlhelp

def help(*tmp):
	print """\
Usage:
	ser_domain [options...] [--] [command] [param...]

%s

Commands & parameters:
	ser_domain add       <domain> <did>
	ser_domain canonical [domain]
	ser_domain change    [domain] [-F flags]
	ser_domain disable   [domain]
	ser_domain enable    [domain]
	ser_domain rm        [domain]
	ser_domain purge
	ser_domain show      [domain]
""" % serctl.ctlhelp.options()


def show(domain=None, did=None, **opts):
	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	u = Domain(opts['DB_URI'])
	uri_list, desc = u.show(domain, cols=cols, raw=numeric, limit=limit)

	tabprint(uri_list, desc, rsep, lsep, astab)

def add(domain, did, **opts):
	force = opts['FORCE']
	flags = opts['FLAGS']

	u = Domain(opts['DB_URI'])
	u.add(domain, did, flags=flags, force=force)

def _change(domain=None, **opts):
	no_all(opts, domain)

	force = opts['FORCE']
	flags = opts['FLAGS']

	u = Domain(opts['DB_URI'])
	u.change(domain, flags=flags, force=force)

def change(domain=None, **opts):
	return _change(domain, opts)

def rm(domain=None, **opts):
	no_all(opts, domain)

	force = opts['FORCE']

	u = Domain(opts['DB_URI'])
	u.rm(domain, force=force)

def enable(domain=None, **opts):
	no_all(opts, domain)

	force = opts['FORCE']

	u = Domain(opts['DB_URI'])
	u.enable(domain, force=force)

def disable(domain=None, **opts):
	no_all(opts, domain)

	force = opts['FORCE']

	u = Domain(opts['DB_URI'])
	u.disable(domain, force=force)

def canonical(domain, **opts):
	opts['FLAGS'] = '+c'
	return _change(domain, opts)

def purge(**opts):
	u = Domain(opts['DB_URI'])
	u.purge()


class Domain:
	T_DOM = 'domain'
	C_DOM = ('did', 'domain', 'last_modified', 'flags')
	I_DOM = idx_dict(C_DOM) # column index dict
	F_DOM = I_DOM['flags']  # flags column index 

	T_DATTR = 'domain_attrs'
	C_DATTR = ('did', 'name', 'type', 'value', 'flags')
	I_DATTR = idx_dict(C_DATTR)
	F_DATTR = I_DATTR['flags']

	T_URI   = 'uri'
	T_CRED  = 'credentials' 

	def __init__(self, dburi):
		self.dburi = dburi
		self.db    = DBany(dburi)

	def _col_idx(self, cols):
		idx = []
		for col in cols:
			try:
				i = self.I_DOM[col]
			except KeyError:
				raise Error (ENOCOL, col)
			idx.append(i)
		return tuple(idx)

	def _full_cond(self, row):
		cnd = ['and', CND_NO_DELETED]
		for i in range(len(self.C_DOM)):
			cnd.append(('=', self.C_DOM[i], row[i]))
		return tuple(cnd)

	def _cond(self, domain=None, did=None, all=False):
		cnd =  ['and']
		if not all:
			cnd.append(CND_NO_DELETED)
		err = []
		if domain is not None:
                        cnd.append(('=', 'domain', domain))
                        err.append('domain=' + domain)
		if did is not None:
                        cnd.append(('=', 'did', did))
                        err.append('did=' + did)
		err = ' '.join(err)
		return (cnd, err)

	def _get_dids(self, domain):
		cnd, err = self._cond(domain)
		rows = self.db.select(self.T_DOM, ('domain', 'did'), cnd)
		dids = {}
		for row in rows:
			dom = row[0]
			did = row[1]
			if dids.has_key(did):
				dids[did].append(domain)
			else:
				dids[did] = [domain]
		return dids, err

	def show(self, domain=None, cols=None, raw=False, limit=0):
		if not cols:
			cols = self.C_DOM
		cidx = self._col_idx(cols)

		cnd, err = self._cond(domain, all=True)
		rows = self.db.select(self.T_DOM, self.C_DOM, cnd, limit)
		new_rows = []
		for row in rows:
			if not raw:
				row[self.F_DOM] = flag_syms(row[self.F_DOM])
			new_row = []
			for i in cidx:
				new_row.append(row[i])
			new_rows.append(new_row)
		desc = self.db.describe(self.T_DOM)
		desc = [ desc[i] for i in cols ]
		return new_rows, desc

	def default_flags(self):
		return str(LOAD_SER | FOR_SERWEB)

	def add(self, domain, did, flags=None, force=False):
		dflags = self.default_flags()
		fmask  = parse_flags(flags)
		flags  = new_flags(dflags, fmask)
		canonical = is_canonical(flags)

		# exist doamin?
		cnd, err = self._cond(domain)
		rows = self.db.select(self.T_DOM, 'flags', cnd, limit=1)
		if rows:
			if not force:
				raise Error (EDUPL, domain)
			self.rm(domain, force=True)

		self._set_dra_if_not_set(did, domain)

		# update canonical flag
		cnd, err = self._cond(did=did)
		rows = self.db.select(self.T_DOM, self.C_DOM, cnd)
		canon_exist = False
		for row in rows:
			if not is_canonical(row[self.F_DOM]):
				continue
			if not canonical:
				canon_exist = True
				break
			f = clear_canonical(row[self.F_DOM])
			cnd = self._full_cond(row)
			self.db.update(self.T_DOM, {'flags':f}, cnd)
		if not canonical and not canon_exist:
			flags = set_canonical(flags)

		# add new domain
		stamp = timestamp()
		ins = { 'did' : did, 'domain' : domain, \
		        'last_modified' : stamp, 'flags' : flags }
		self.db.insert(self.T_DOM, ins)

	def change(self, domain=None, flags=None, force=False):
		upd = {}
		fmask = parse_flags(flags)
		nflags = new_flags(0, fmask)
		canonical = is_canonical(nflags)

		# get did
		dids, err = self._get_dids(domain)
		if not dids:
			if force:
				return
			raise Error (ENOREC, err)

		# clear canon
		if canonical:
			for did in dids.keys():
				cnd, err = self._cond(did=did)
				cnd.append(CND_CANONICAL)
				rows = self.db.select(self.T_DOM, self.C_DOM, \
				  cnd)
				for row in rows:
					nf = clear_canonical(row[self.F_DOM])
					cnd = self._full_cond(row)
					upd = {'flags':nf}
					self.db.update(self.T_DOM, upd, cnd)

		# update flags
		cnd, err = self._cond(domain)
		rows = self.db.select(self.T_DOM, self.C_DOM, cnd)
		if not rows and not force:
			raise Error (ENOREC, err)
		for row in rows:
			nf = new_flags(row[self.F_DOM], fmask)
			cnd = self._full_cond(row)
			self.db.update(self.T_DOM, {'flags':nf}, cnd)

	def _rm_last_did(self, did, doms, force):
		cnd = ['and', ('=', 'did', did), CND_NO_DELETED]
		for dom in doms:
			cnd.append(('!=', 'domain', dom))
		rows = self.db.select(self.T_DOM, 'did', cnd, limit=1)
		if rows:
			return

		udel = cdel = False

		# did in uri?
		cnd = ('and', ('=', 'did', did), CND_NO_DELETED)
		rows = self.db.select(self.T_URI, 'did', cnd, limit=1)
		if rows:
			if not force:
				raise Error (EDOMAIN, did)
			udel = True

		# domains in cred
		cnd = ['or', ]
		for dom in doms:
			cnd.append(('=', 'realm', dom))
		cnd = ('and', CND_NO_DELETED, cnd)
		rows = self.db.select(self.T_CRED, 'realm', cnd)
		if rows:
			if not force:
				raise Error (EDOMAIN, rows[0])
			cdel = True
		
		if udel:
			udb = Uri(self.dburi)
			udb.rm(did=did, force=True)
			del(udb)
		if cdel:
			cdb = Cred(self.dburi)
			for dom in doms:
				cdb.rm(realm=dom, force=True)
			del(cdb)

	def rm(self, domain=None, force=False):
		dids, err = self._get_dids(domain)
		if not dids:
			if force:
				return
			raise Error (ENOREC, err)
		for did, doms in dids.items():
			self._rm_last_did(did, doms, force)
			self._rm_dra(did, doms)

		# is canonical?
		canon_deleted = True
		if domain is not None:
			cnd, err = self._cond(domain)
			cnd.append(CND_CANONICAL)
			rows = self.db.select(self.T_DOM, self.C_DOM, cnd, \
			  limit=1)
			if not rows:
				canon_deleted = False
		# rm 
		cnd, err = self._cond(domain)
		rows = self.db.select(self.T_DOM, self.C_DOM, cnd)
		if not rows and not force:
			raise Error (ENOREC, err)
		for row in rows:
			nf = set_deleted(row[self.F_DOM])
			cnd = self._full_cond(row)
			self.db.update(self.T_DOM, {'flags': nf}, cnd)

		# set new canon flag
		if canon_deleted:
			for did, doms in dids.items():
				cnd, err = self._cond(did=did)
				rows = self.db.select(self.T_DOM, self.C_DOM, \
				  cnd, limit=1)
				if rows:
					nf = set_canonical(rows[0][self.F_DOM])
					cnd = self._full_cond(rows[0])
					upd = {'flags': nf}
					self.db.update(self.T_DOM, upd, cnd)

	def purge(self):
		self.db.delete(self.T_DOM, CND_DELETED)
		self.db.delete(self.T_DATTR, CND_DELETED)

	def enable(self, domain=None, force=False):
		return self.change(domain, flags='-d', force=force)

	def disable(self, domain=None, force=False):
		return self.change(domain, flags='+d', force=force)

	def get_did(self, domain, force=False):
		cnd, err = self._cond(domain, all=True)
		rows = self.db.select(self.T_DOM, ('did',), cnd, limit=1)
		if not rows:
			if force:
				return None
			else:
				raise Error (ENODOMAIN, err)
		did = rows[0][0]
		return did

	def _default_dra_flags(self): # default flags for digest_realm attr
		return '0'

	def _full_dra_cond(self, row):
		cnd = ['and', CND_NO_DELETED]
		for i in range(len(self.C_DATTR)):
			cnd.append(('=', self.C_DATTR[i], row[i]))
		return tuple(cnd)

	def _set_dra_if_not_set(self, did, domain):
		# if set return
		cnd = ('and', CND_NO_DELETED, ('=', 'did', did), \
		  ('=', 'name','digest_realm'))
		rows = self.db.select(self.T_DATTR, 'did', cnd, limit=1)
		if rows:
			return

		# set new
		flags = self._default_dra_flags()
		ins = {'did': did, 'name': 'digest_realm', 'value': domain, \
		  'type': 2, 'flags': flags }
		self.db.insert(self.T_DATTR, ins)

	def _rm_dra(self, did, doms):
		cnd = ['or', ]
		for dom in doms:
			cnd.append(('=', 'value', dom))
		cnd = ('and', CND_NO_DELETED, ('=', 'did', did), \
		  ('=', 'name','digest_realm'), cnd)

		rows = self.db.select(self.T_DATTR, self.C_DATTR, cnd)
		for row in rows:
			nf = set_deleted(row[self.F_DATTR])
			cnd = self._full_dra_cond(row)
			self.db.update(self.T_DATTR, {'flags': nf}, cnd)
