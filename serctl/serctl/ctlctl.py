#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlctl.py,v 1.23 2006/03/31 16:02:44 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.flag      import IS_FROM
from serctl.ctluri    import Uri
from serctl.ctlcred   import Cred
from serctl.ctldomain import Domain
from serctl.ctluser   import User
from serctl.error     import Error, ENOARG, EINVAL, ENOSYS, EDOMAIN, \
                             ENODOMAIN, warning, EDUPL, ENOUSER, ERPC
from serctl.ctlrpc    import any_rpc
from serctl.options   import CMD, CMD_ADD, CMD_RM, CMD_PASSWORD, CMD_SHOW
from serctl.uri       import split_sip_uri
from serctl.utils     import show_opts, var2tab, tabprint, dict2tab
import serctl.ctlhelp, xmlrpclib

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
	ser_ctl alias  add  <username> <alias>
	ser_ctl alias  rm   <alias>
	ser_ctl alias  show
	ser_ctl domain add  <domain>
	ser_ctl domain rm   <domain>
	ser_ctl password    <uri> <password>
	ser_ctl user   add  <uri> <password>
	ser_ctl user   rm   <username>
	ser_ctl user   show
	ser_ctl usrloc add  <uri> <contact>
	ser_ctl usrloc show <uri>
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

def purge(**opts):
	for c in (Uri, Cred, Domain, User):
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

def domain(command, domain, **opts):
	cmd = CMD.get(command)
	if cmd == CMD_ADD:
		d = Domain_ctl(opts['DB_URI'], any_rpc(opts))
		d.add(domain)
	elif cmd == CMD_RM:
		d = Domain_ctl(opts['DB_URI'], any_rpc(opts))
		d.rm(domain)
	else:
		raise Error (EINVAL, command)
		
def user(command, uri_user=None, password=None, **opts):
	return _user(command, uri_user, password, opts)

def _user(command, uri_user, password, opts):
	cmd = CMD.get(command)
	if cmd == CMD_ADD:
		if uri_user is None:
			raise Error (ENOARG, 'uri')
		password = opts.get('PASSWORD', password)
		if password is None:
			raise Error (ENOARG, 'password')
		u = User_ctl(opts['DB_URI'], any_rpc(opts))
		u.add(uri_user, password)
	elif cmd == CMD_RM:
		if uri_user is None:
			raise Error (ENOARG, 'username')
		u = User_ctl(opts['DB_URI'], any_rpc(opts))
		u.rm(uri_user)
	elif cmd == CMD_SHOW:
		cols, numeric, limit, rsep, lsep, astab = show_opts(opts)
		u = User_ctl(opts['DB_URI'], any_rpc(opts))
		ret, desc = u.show(raw=numeric)
		tabprint(ret, desc, rsep, lsep, astab)
	elif cmd == CMD_PASSWORD:
		if uri_user is None:
			raise Error (ENOARG, 'uri')
		password = opts.get('PASSWORD', password)
		if password is None:
			raise Error (ENOARG, 'password')
		u = User_ctl(opts['DB_URI'], any_rpc(opts))
		u.passwd(uri_user, password)
	else:
		raise Error (EINVAL, command)

def password(uri, password=None, **opts):
	return _user(CMD_PASSWORD, uri, password, opts)

def alias(command, user_alias=None, alias=None, **opts):
	cmd = CMD.get(command)
	if cmd == CMD_ADD:
		if user_alias is None:
			raise Error (ENOARG, 'username')
		if alias is None:
			raise Error (ENOARG, 'alias')
		a = Alias_ctl(opts['DB_URI'], any_rpc(opts))
		a.add(user_alias, alias)
	elif cmd == CMD_RM:
		if user_alias is None:
			raise Error (ENOARG, 'alias')
		a = Alias_ctl(opts['DB_URI'], any_rpc(opts))
		a.rm(user_alias)
	elif cmd == CMD_SHOW:
		cols, numeric, limit, rsep, lsep, astab = show_opts(opts)
		u = Alias_ctl(opts['DB_URI'], any_rpc(opts))
		ret, desc = u.show(raw=numeric)
		tabprint(ret, desc, rsep, lsep, astab)
	else:
		raise Error (EINVAL, command)
		
def usrloc(command, uri, contact=None, **opts):
	cmd = CMD.get(command)
	if cmd == CMD_ADD:
		if contact is None:
			raise Error (ENOARG, 'contact')
		u = Usrloc_ctl(opts['DB_URI'], any_rpc(opts))
		u.add(uri, contact)
	elif cmd == CMD_RM:
		u = Usrloc_ctl(opts['DB_URI'], any_rpc(opts))
		u.rm(uri, contact)
	elif cmd == CMD_SHOW:
		cols, numeric, limit, rsep, lsep, astab = show_opts(opts)
		u = Usrloc_ctl(opts['DB_URI'], any_rpc(opts))
		ret = u.show(uri)
		if type(ret) == dict:	# FIX: Is this a bug in usrloc SER code?
			ret = [ret]
		ret = [ (str(i['contact']), str(i['expires']), str(i['q']) ) for i in ret ]
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
	ret = rpc.core_kill(sig)

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

	rpc = any_rpc(opts)

	exists = [ i for i in rpc.ser.system.listMethods() if i[-7:] == '.reload' ]

	for fn in exists:
		try:
			rpc.raw_cmd(fn)
		except xmlrpclib.Fault, inst:
                        warning("Function '%s' fail: " % fn +  repr(inst))
		except Error, inst:
			if inst.err == ERPC:
				warning('Domain reloading fail: ' + str(inst.text))
			else:
				raise

def methods(**opts):
	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = any_rpc(opts)
	ret = rpc.system_listmethods()

	ret, desc = var2tab(ret, 'methods')
	tabprint(ret, desc, rsep, lsep, astab)


class Domain_ctl:
	def __init__(self, dburi, rpc):
		self.db  = dburi
		self.rpc = rpc

	def add(self, domain):
		dom = Domain(self.db)
		did = dom.get_did(domain, True)
		if did is not None:
			raise Error (EDOMAIN, domain)
		dom.add(domain, domain)
		self._reload()

	def rm(self, domain):
		dom = Domain(self.db)
		cred = Cred(self.db)
		did = dom.get_did(domain, True)
		if did is None:
			raise Error (ENODOMAIN, domain)
		try:
			cred.rm(realm=domain)
		except:
			pass
		dom.rm(domain)
		dom.purge()
		cred.purge()
		self._reload()

	def _reload(self):
		try:
			self.rpc.ser.domain.reload()
		except xmlrpclib.Fault, inst:
			warning('Domain reloading fail: ' +  repr(inst))
		except Error, inst:
			if inst.err == ERPC:
				warning('Domain reloading fail: ' + str(inst.text))
			else:
				raise

class User_ctl:
	def __init__(self, dburi, rpc):
		self.db  = dburi
		self.rpc = rpc

	def show(self, raw=False):
		user = User(self.db)
		uids, tmp = user.show(cols=['uid'])
		uids = [ i[0] for i in uids ]
		uri = Uri(self.db)
		ret = []
		for uid in uids:
			uris, tmp = uri.show(uid=uid, cols=['username', 'did', 'flags'], raw=raw, canonical=True)
			for u in uris:
				ret.append((uid, '@'.join(u[:2]), u[2]))
		desc = (('uid',),('uri',), ('flags',))
		return ret, desc

	def add(self, uri, password):
		dom = Domain(self.db)
		usr = User(self.db)
		ur  = Uri(self.db)
		cred = Cred(self.db)
		user, domain = split_sip_uri(uri)
		did = dom.get_did(domain, True)
		if did is None:
			raise Error (ENODOMAIN, domain)
		exist = ur.get_uids(uri, force=True)
		if exist != []:
			raise Error (EDUPL, uri)
		usr.add(user)
		ur.add(uri, user)
		cred.add(user, domain, user, password)
		
	def rm(self, user):
		usr = User(self.db)
		uri = Uri(self.db)
		cred = Cred(self.db)
		try:
			cred.rm(uid=user)
		except:
			pass
		uri.rm_user(user, quiet=True)
		usr.rm(user)
		cred.purge()
		uri.purge()
		usr.purge()

	def passwd(self, uri, password):
		user, domain = split_sip_uri(uri)
		cred = Cred(self.db)
		cred.change(username=user, realm=domain, password=password)

class Alias_ctl:
	def __init__(self, dburi, rpc):
		self.db  = dburi
		self.rpc = rpc

	def show(self, raw=False):
		user = User(self.db)
		uids, tmp = user.show(cols=['uid'])
		uids = [ i[0] for i in uids ]
		uri = Uri(self.db)
		ret = []
		for uid in uids:
			uris, tmp = uri.show(uid=uid, cols=['username', 'did', 'flags'], raw=raw)
			for u in uris:
				ret.append((uid, '@'.join(u[:2]), u[2]))
		desc = (('uid',),('uri',), ('flags',))
		return ret, desc


	def add(self, username, alias):
		ur = Uri(self.db)
		uid, did = ur.get_alias_info(username, canonical=True)
		uri = alias + '@' + did
		exist = ur.get_uids(uri, force=True)
		if exist != []:
			raise Error (EDUPL, uri)
		ur.add(uri, username)

	def rm(self, alias):
		ur = Uri(self.db)
		uid, did = ur.get_alias_info(alias, canonical=False)
		uri = alias + '@' + did
		ur.rm(uri=uri, uid=uid, did=did)
		ur.purge()

class Usrloc_ctl:
	def __init__(self, dburi, rpc):
		self.db  = dburi
		self.rpc = rpc

	def _get_uid(self, uri):
		ur = Uri(self.db)
		uids = ur.get_uids(uri, flags=[IS_FROM])
		if not uids:
			raise Error (ENOUSER, 'For uri=%s (with TO_FLAG set)' % uri)
		return uids[0]

	def show(self, uri):
		uid = self._get_uid(uri)
		return self.rpc.ser.usrloc.show_contacts('location', uid)

	def add(self, uri, contact):
		uid = self._get_uid(uri)
		self.rpc.ser.usrloc.add_contact('location', uid, contact, 0, 1, 128)

	def rm(self, uri, contact=None):
		uid = self._get_uid(uri)
		if contact is None:
			self.rpc.ser.usrloc.delete_uid('location', uid)
		else:
			self.rpc.ser.usrloc.delete_contact('location', uid, contact)
