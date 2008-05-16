#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlctl.py,v 1.49 2008/05/16 13:30:29 kozlik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.flag      import IS_FROM, CND_NO_DELETED
from serctl.ctluri    import Uri
from serctl.ctlcred   import Cred
from serctl.ctldomain import Domain
from serctl.ctlattr   import Domain_attrs, User_attrs, Global_attrs, \
                             _set
from serctl.ctluser   import User
from serctl.dbany     import DBany
from serctl.error     import Error, ENOARG, EINVAL, ENOSYS, EDOMAIN, \
                             ENODOMAIN, warning, EDUPL, ENOUSER, ERPC, \
                             ENOREC, ENOCANON
from serctl.ctlrpc    import any_rpc, multi_rpc
from serctl.uniqid    import get_idtype, ID_URI, id
from serctl.options   import CMD, CMD_ADD, CMD_RM, CMD_PASSWORD, CMD_SHOW, \
                             CMD_SET
from serctl.uri       import split_sip_uri
from serctl.utils     import show_opts, var2tab, tabprint, dict2tab, \
                             arg_attrs, uniq, errstr, cond, get_password
import serctl.ctlhelp, xmlrpclib, sys

# xml-rpc method used in stats function (+ all *.stats)
STATS = [ \
	'core.uptime', 
	'core.shmmem',
	'core.tcp_info',
]


def help(*tmp):
	print """\
Usage:
	ser_ctl [options...] [--] [command] [param...]

%s

Commands & parameters:
  - user and domain administration:
	ser_ctl alias  add  <uri> <alias> [alias...]
	ser_ctl alias  rm   <alias> [alias...]
	ser_ctl attrs  set  <uri> <attr>=<value> [attr=value]...
	ser_ctl domain add  <domain> [domain_alias...]
	ser_ctl domain rm   [domain...]
	ser_ctl domain show [domain]
	ser_ctl password    <uri> [-p] [password]
	ser_ctl user   add  <uri> [-p password] [alias...] 
	ser_ctl user   rm   <uri>
	ser_ctl user   show <uri>
	ser_ctl usrloc add  <uri> <contact> [expires=<seconds>] [q=<q>] [flags=<flags>] 
	ser_ctl usrloc show <uri> [contact]
	ser_ctl usrloc rm   <uri> [contact] 

  - remove database records marked as deleted:
	ser_ctl purge

  - SER control, info and statistics:
	ser_ctl kill  [sig]
	ser_ctl ps
	ser_ctl reload
	ser_ctl stat [module_name...]
	ser_ctl uptime
	ser_ctl version
        ser_ctl list_methods
        ser_ctl list_tls

  - miscelaneous:
	ser_ctl publish <uid> <file_with_PIDF_doc> <expires_in_sec> [etag]
""" % serctl.ctlhelp.options()

def attrs(command, uri, *attrs, **opts):
	cmd = CMD.get(command)
	if cmd == CMD_SET:
		attrs = ('user='+uri,) + attrs
		return _set(attrs, opts)
	else:
		raise Error (EINVAL, command)

def purge(**opts):
	for c in (Uri, Cred, Domain, User, Domain_attrs, User_attrs, Global_attrs):
		o = c(opts['DB_URI'])
		o.purge()
		del(o)

def publish(uid, file_with_PIDF_doc, expires_in_sec, etag=None, **opts):
	expires = int(expires_in_sec)

	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = any_rpc(opts)
	fh = open(file_with_PIDF_doc)
	doc = fh.read()
	fh.close()
	if etag:
		ret = rpc.ser.pa.publish('registrar', uid, doc, expires, etag)
	else:
		ret = rpc.ser.pa.publish('registrar', uid, doc, expires)
	if astab:
		ret, desc = var2tab(ret)
		tabprint(ret, desc, rsep, lsep, astab)
	else:
		print repr(ret)


def domain(command, *domain, **opts):
	force = opts['FORCE']
	cmd = CMD.get(command)
	idtype = get_idtype(opts)
	if cmd == CMD_ADD:
		if len(domain) < 1:
			raise Error (ENOARG, 'domain')
		d = Domain_ctl(opts['DB_URI'], any_rpc(opts))
		d.add(domain[0], domain[1:], idtype, force)
	elif cmd == CMD_RM:
		d = Domain_ctl(opts['DB_URI'], any_rpc(opts))
		d.rm(domain, force)
	elif cmd == CMD_SHOW:
		if not domain:
			domain = None
		else:
			domain = domain[0]
		cols, fformat, limit, rsep, lsep, astab = show_opts(opts)
		u = Domain(opts['DB_URI'])
		if opts['DID']:
			uri_list, desc = u.show_did(domain, cols=cols, fformat=fformat, limit=limit)
		elif opts['DEPTH']:
			uri_list, desc = u.show_did_for_domain(domain, cols=cols, fformat=fformat, limit=limit)
		else:
			uri_list, desc = u.show_domain(domain, cols=cols, fformat=fformat, limit=limit)
		tabprint(uri_list, desc, rsep, lsep, astab)
	else:
		raise Error (EINVAL, command)


def user(command, uri, *aliases, **opts):
	force = opts['FORCE']
	cmd = CMD.get(command)
	idtype = get_idtype(opts)
	if cmd == CMD_ADD:
		prompt='Please, enter password for the new subscriber.\nPassword: '
		password = get_password(opts, prompt=prompt)
		u = User_ctl(opts['DB_URI'], multi_rpc(opts))
		u.add(uri, aliases, password, idtype, force)
	elif cmd == CMD_RM:
		u = User_ctl(opts['DB_URI'], multi_rpc(opts))
		u.rm(uri, idtype, force)
	elif cmd == CMD_SHOW:
		cols, fformat, limit, rsep, lsep, astab = show_opts(opts)
		u = User_ctl(opts['DB_URI'], multi_rpc(opts))
		ret, desc = u.show(uri, cols, fformat, limit)
		tabprint(ret, desc, rsep, lsep, astab)
	else:
		raise Error (EINVAL, command)

def password(uri, password=None, **opts):
	force = opts['FORCE']
	prompt="Please, enter new subscriber's password.\nPassword: "
	password = get_password(opts, prompt=prompt)
	u = User_ctl(opts['DB_URI'], multi_rpc(opts))
	u.passwd(uri, password, force)

def alias(command, *uri, **opts):
	force = opts['FORCE']
	cmd = CMD.get(command)
	if cmd == CMD_ADD:
		if len(uri) < 1:
			raise Error (ENOARG, 'uri')
		if len(uri) < 2:
			raise Error (ENOARG, 'alias')
		a = Alias_ctl(opts['DB_URI'], multi_rpc(opts))
		a.add(uri[0], uri[1:], force)
	elif cmd == CMD_RM:
		if len(uri) < 1:
			raise Error (ENOARG, 'alias')
		a = Alias_ctl(opts['DB_URI'], multi_rpc(opts))
		a.rm(uri, force)
	else:
		raise Error (EINVAL, command)
		
def usrloc(command, uri, contact=None, *args, **opts):
	ad, al = arg_attrs(args)
	table = opts['UL_TABLE']
	q = float(ad.get('q', 1))
	expires = ad.get('expires')
	if expires is not None:
		expires = int(expires)
	flags = ad.get('flags')
	if flags is not None:
		flags = int(flags)

	# LB hack
	if opts['SER_URI'][:4] == 'http':
		ur = Uri(opts['DB_URI'])
		curi = ur.canonize(uri)
		del(ur)
		if opts['SER_URI'][-1:] != '/':
			opts['SER_URI'] = opts['SER_URI'] + '/'
		opts['SER_URI'] = opts['SER_URI'] + 'sip:' + curi

	cmd = CMD.get(command)
	if cmd == CMD_ADD:
		if contact is None:
			raise Error (ENOARG, 'contact')
		u = Usrloc_ctl(opts['DB_URI'], any_rpc(opts))
		u.add(uri, contact, table, expires, q, flags)
	elif cmd == CMD_RM:
		u = Usrloc_ctl(opts['DB_URI'], any_rpc(opts))
		u.rm(uri, contact, table)
	elif cmd == CMD_SHOW:
		cols, numeric, limit, rsep, lsep, astab = show_opts(opts)
		u = Usrloc_ctl(opts['DB_URI'], any_rpc(opts))
		ret = u.show(uri, table)
		if type(ret) == dict:	# FIX: Is this a bug in usrloc SER code?
			ret = [ret]
		ret = [ (str(i['contact']), str(i['expires']), str(i['q'])) for i in ret ]
		desc = (('contact',), ('expires',), ('q',))
		tabprint(ret, desc, rsep, lsep, astab)
	else:
		raise Error (EINVAL, command)

def ps(**opts):
	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = any_rpc(opts)
	ret = rpc.core_ps()

	desc = [ ('id', None, ''), ('process description', None, '') ]
	ret = [ (str(a), b) for a, b in ret ]
	tabprint(ret, desc, rsep, lsep, astab)

def version(**opts):
	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = any_rpc(opts)
	ret = rpc.core_version()

	ret, desc = var2tab(ret, 'version')
	tabprint(ret, desc, rsep, lsep, astab)

def uptime(**opts):
	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = any_rpc(opts)
	ret = rpc.core_uptime()

	ret, desc = dict2tab(ret, ('uptime', 'up_since', 'now'))
	tabprint(ret, desc, rsep, lsep, astab)

def list_tls(**opts):
	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = any_rpc(opts)
	ret = rpc.tls_list()

	desc = [ ('ID', None, ''),     ('Timeout', None, ''), \
	         ('Source', None, ''), ('Destination', None, ''), \
	         ('TLS', None, '') ]
	ret = [ (str(s['id']), str(s['timeout']), \
	        s['src_ip'] + ':' + str(s['src_port']), \
	        s['dst_ip'] + ':' + str(s['dst_port']), s['tls']) for s in ret ]
	tabprint(ret, desc, rsep, lsep, astab)

def kill(sig=15, **opts):
	sig = int(sig)

	rpc = any_rpc(opts)
	return rpc.core_kill(sig)

def stat(*modules, **opts):
	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = any_rpc(opts)

	estats = [ i for i in rpc.ser.system.listMethods() if i[-6:] == '.stats' ]
	display = [ i for i in rpc.ser.system.listMethods() if i in STATS ] + estats

	if modules:
		display = [ i for i in display if i.split('.')[0] in modules ]

	st = []
	for fn in display:
		ret = rpc.cmd(fn)
		if fn == 'usrloc.stats':	# FIX: Is this usrloc.stats processing correct?
			d = {}
			for x in ret:
				name = x['domain']
				for k, v in x.items():
					if k == 'domain': continue
					d[name + ' : ' + k] = v
			ret = d
		if type(ret) is dict:
			for k, v in ret.items():
				st.append([fn, str(k), str(v).strip()])
		elif type(ret) in [tuple, list]:
			for v in ret:
				st.append([fn, '', str(v).strip()])
		else:
			st.append([fn,'',str(ret).strip()])
	dsc = [('function', None, ''), ('name', None, ''), ('value', None, '')]
	tabprint(st, dsc, rsep, lsep, astab)

def reload(**opts):

	rpc = multi_rpc(opts)
	rpc.reload()

def methods(**opts):
	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = any_rpc(opts)
	ret = rpc.system_listmethods()

	ret, desc = var2tab(ret, 'methods')
	tabprint(ret, desc, rsep, lsep, astab)


class Domain_ctl:
	def __init__(self, dburi, rpc, db=None):
		self.dburi = dburi
		if db is None:
			self.db = DBany(dburi)
		else:
			self.db = db
		self.rpc = rpc

	def add(self, domain, aliases=[], idtype=ID_URI, force=False):
		do = Domain(self.dburi, self.db)

		did = id(domain, idtype)
		if do.exist_did(did) and not force:
			raise Error (EDOMAIN, errstr(did=did))

		if do.exist_domain(domain) and not force:
			raise Error (EDOMAIN, domain)

		ualiases = uniq(aliases)
		naliases = []
		for alias in ualiases:
			if do.exist(did, alias):
				if force: continue
				raise Error (EDUPL, errstr(did=did, domain=alias))
			else:
				naliases.append(alias)

		do.add(did, domain, None, force)
		for alias in naliases:
			do.add(did, alias, None, force)
		self._reload()

	def rm(self, domains, force=False):
		do = Domain(self.dburi, self.db)
		da = Domain_attrs(self.dburi, self.db)
		ur = Uri(self.dburi, self.db)
		cr = Cred(self.dburi, self.db)
		domains = uniq(domains)

		doms = []
		for d in domains:
			try:
				did = do.get_did(d)
			except:
				if not force:
					raise Error (ENODOMAIN, d)
			doms.append((did, d))

		for i, d in doms:
			try:
				ur.rm_did(i, force=force)
			except:
				pass
			try:
				cr.rm_realm(d, force=force)
			except:
				pass
			do.rm(i, d, force=force)

		do.purge()
		ur.purge()
		cr.purge()
		da.purge()
		self._reload()

	def _reload(self):
		self.rpc.ser.domain.reload()

class User_ctl:
	def __init__(self, dburi, rpc, db=None):
		self.dburi = dburi
		if db is None:
			self.db = DBany(dburi)
		else:
			self.db = db
		self.rpc = rpc

	def show(self, uri, cols=None, fformat=False, limit=0):
		ur = Uri(self.dburi, self.db)
		us = User(self.dburi, self.db)
		ua = User_attrs(self.dburi, self.db)
		do = Domain(self.dburi, self.db)
		cr = Cred(self.dburi, self.db)
		try:
			uids = ur.get_uids(uri)
		except:
			try:
				uids = ur.get_uids_for_username(uri)
			except:
				try:
					uids = us.get(uri)
				except:
					u, d = split_sip_uri(uri)
					try:
						uids = [cr.get_uid(u, d)]
					except:
						try:
							uids = cr.get_uids_for_username(u)
						except:
							try:
								uids = cr.get_uids_for_username(uri)
							except:
								uids = []

		dids = []

		# get uris
		uris = []
		for uid in uids:
			u, d = ur.show_uid(uid, ['uid', 'username', 'did'], fformat=fformat, limit=limit)
			uris += u
		for row in uris:
			dids.append(row[2])

		# get credentials
		creds = []
		for uid in uids:
			u, d = cr.show_uid(uid, ['uid', 'auth_username', 'realm', 'password'], fformat=fformat, limit=limit)
			creds += u
		for row in creds:
			try:
				did = do.get_did(row[2])
			except:
				continue
			dids.append(did)
	
		dids = uniq(dids)
		domains = {}
		for did in dids:
			try:
				dom = do.get_domain(did)
			except:
				continue
			domains[did] = dom

		attrs = {}
		for uid in uids:
			ce = cond(CND_NO_DELETED, uid=uid)
			rows = ua.show_cnd(ce, ['name', 'value'], fformat, limit)[0]
			line = []
			for row in rows:
				line.append('%s=%s' % tuple(row))
			attrs[uid] = ', '.join(line)

		# show
		desc = [('uid',), ('source',), ('value',)]
		ret  = [] 
		for u in uris:
			dom = domains.get(u[2])
			uri = '%s@%s' % (u[1], dom)
			uid = u[0]
			ret.append([uid, 'uri', uri ])
		for c in creds:
			cred = 'username=%s realm=%s password=%s' % (c[1], c[2], c[3])
			uid = c[0]
			ret.append([uid, 'credentials', cred ])
		for uid, attr in attrs.items():
			ret.append([uid, 'attr', attr ])

		# limit output
		if limit > 0:
			ret = ret[:limit]
		return ret, desc
			

	def add(self, uri, aliases, password, idtype=ID_URI, force=False):
		do = Domain(self.dburi, self.db)
		da = Domain_attrs(self.dburi, self.db)
		ur = Uri(self.dburi, self.db)
		cr = Cred(self.dburi, self.db)
		us = User(self.dburi, self.db)

		user, domain = split_sip_uri(uri)
		aliases = uniq(aliases)
		try:
			n = aliases.index(uri)
			aliases = aliases[:n] + aliases[n+1:]
		except:
			pass
		aliases = [ split_sip_uri(a) for a in aliases ]

		uid = id(user + '@' + domain, idtype)
		if us.exist(uid):
			if not force:
				raise Error (EDUPL, errstr(uid=uid))
		if cr.exist(user, domain):
			if not force:
				raise Error (EDUPL, errstr(auth_username=user, domain=domain))
		if not do.exist_domain(domain):
			if force:
				i = id(domain, idtype)
				do.add(i, domain, force=force)
			else:
				raise Error (ENODOMAIN, domain)
		did = do.get_did(domain)
		try:
			realm = da.get(did, 'digest_realm')
		except:
			if force:
				realm = domain
			else:
				raise
		for u, d in aliases:
			if not do.exist_domain(d):
				if force:
					i = id(d, idtype)
					do.add(i, d, force=force)
				else:
					raise Error (ENODOMAIN, d)
			did = do.get_did(d)
			if ur.exist_username_did(u, did) and not force:
				raise Error (EDUPL, '%s@%s' % (u, d))
		did = do.get_did(domain)
		if ur.exist(uid, user, did):
			if not force:
				raise Error(EDUPL, errstr(uid=uid, username=user, did=did))

		us.add(uid, force=force)
		cr.add(uid, user, did, realm, password, force=force)
		ur.add(uid, uri, force=force)
		for u, d in aliases:
			uri = '%s@%s' % (u, d)
			try:
				ur.add(uid, uri, force=force)
			except Error, inst:
				warning(str(inst))
		self._reload()

	def rm(self, uri, idtype=ID_URI, force=False):
		us = User(self.dburi, self.db)
		ur = Uri(self.dburi, self.db)
		cr = Cred(self.dburi, self.db)
		try:
			uid = ur.get_uid(uri)
		except:
			if force: return
			raise
		try:
			ur.rm_uid(uid, force=force)
		except Error:
			pass
		try:
			cr.rm_uid(uid, force=force)
		except Error:
			pass
		try:
			us.rm(uid, force=force)
		except Error:
			pass
		us.purge()
		ur.purge()
		cr.purge()
		self._reload()

	def passwd(self, uri, password, force):
		cred = Cred(self.dburi, self.db)
		user, domain = split_sip_uri(uri)
		cred.change(username=user, realm=domain, password=password)
		self._reload()

	def _reload(self):
		# FIX: what fn to call (if needed)
		pass

class Alias_ctl:
	def __init__(self, dburi, rpc, db=None):
		self.dburi = dburi
		if db is None:
			self.db = DBany(dburi)
		else:
			self.db = db
		self.rpc = rpc

	def add(self, uri, aliases, force=False):
		do = Domain(self.dburi, self.db)
		ur = Uri(self.dburi, self.db)
		us = User(self.dburi, self.db)

		try:
			uid = ur.get_uid(uri)
		except:
			if force: return
			raise
		try:
			user, did = ur.uri2id(uri)
		except:
			if force: return
			raise

		if not us.exist(uid):
			if force: return
			raise Error (ENOUSER, user)

		aliases = uniq(aliases)
		try:
			n = aliases.index(uri)
			aliases = aliases[:n] + aliases[n+1:]
		except:
			pass

		for a in aliases:
			if ur.exist_uri(a) and not force:
				raise Error (EDUPL, a)
			u, d =  split_sip_uri(a)
			if not do.exist_domain(d) and not force:
				raise Error (ENODOMAIN, d)
		
		for a in aliases:
			try:
				ur.add(uid, a, force=force)
			except Error, inst:
				warning(str(inst))
		self._reload()

	def rm(self, aliases, force=False):
		ur = Uri(self.dburi, self.db)

		aliases = uniq(aliases)
		for a in aliases:
			if not ur.exist_uri(a) and not force:
				raise Error (ENOREC, a)

		for a in aliases:
			try:
				ur.rm_uri(a, force=force)
			except Error, inst:
				warning(str(inst))

		ur.purge()
		self._reload()

	def _reload(self):
		# FIX: what fn to call (if needed)
		pass

class Usrloc_ctl:
	def __init__(self, dburi, rpc, db=None):
		self.dburi = dburi
		if db is None:
			self.db = DBany(dburi)
		else:
			self.db = db
		self.rpc = rpc

	def _get_uid(self, uri):
		ur = Uri(self.dburi, self.db)
		uid = ur.get_uid(uri)
		return uid

	def show(self, uri, table='location'):
		uid = self._get_uid(uri)
		return self.rpc.ser.usrloc.show_contacts(table, uid)

	def add(self, uri, contact, table='location', expires=None, q=1.0, flags=None):
		uid = self._get_uid(uri)
		if expires is None:
			if flags is None:
				flags = 128
			expires = 0
		else:
			if flags is None:
				flags = 0
		self.rpc.ser.usrloc.add_contact(table, uid, contact, expires, q, flags)

	def rm(self, uri, contact=None, table='location'):
		uid = self._get_uid(uri)
		if contact is None:
			self.rpc.ser.usrloc.delete_uid(table, uid)
		else:
			self.rpc.ser.usrloc.delete_contact(table, uid, contact)
