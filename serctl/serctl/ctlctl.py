#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlctl.py,v 1.14 2006/03/03 18:28:04 janakj Exp $
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
                             ENODOMAIN, warning, EDUPL, ENOUSER
from serctl.options   import CMD_FLUSH, CMD_PURGE, OPT_DATABASE, CMD_PUBLISH, \
                             OPT_SER_URI, CMD, CMD_HELP, CMD_DOMAIN, CMD, \
                             CMD_ADD, CMD_RM, OPT_SSL_KEY, OPT_SSL_CERT, \
                             CMD_USER, CMD_PASS, CMD_ALIAS, CMD_PS, \
                             CMD_VERSION, CMD_UPTIME, CMD_KILL, CMD_STAT, \
                             CMD_RELOAD, CMD_USRLOC, CMD_SHOW, CMD_LIST_TLS
from serctl.ctlrpc    import Xml_rpc
from serctl.uri       import split_sip_uri
from serctl.utils     import show_opts, var2tab, tabprint, dict2tab
import serctl.ctlhelp, xmlrpclib

def main(args, opts):
	if len(args) < 3:
		print help(args, opts)
		return
	try:
		cmd = CMD[args[2]]
	except KeyError:
		raise Error (EINVAL, args[2])
	db  = opts[OPT_DATABASE]
	ser = opts[OPT_SER_URI]
	ssl = (opts[OPT_SSL_KEY], opts[OPT_SSL_CERT])

	if   cmd == CMD_ALIAS:
		ret = alias(db, ser, ssl, args[3:], opts)
	elif   cmd == CMD_FLUSH:
		ret = flush(ser, ssl, args[3:], opts)
	elif cmd == CMD_DOMAIN:
		ret = domain(db, ser, ssl, args[3:], opts)
	elif cmd == CMD_KILL:
		ret = kill(ser, ssl, args[3:], opts)
	elif cmd == CMD_HELP:
		print help(args, opts)
		return
	elif cmd == CMD_PASS:
		ret = password(db, ser, ssl, args[3:], opts)
	elif cmd == CMD_PS:
		ret = ps(ser, ssl, args[3:], opts)
	elif cmd == CMD_PUBLISH:
		ret = publish(ser, ssl, args[3:], opts)
	elif cmd == CMD_PURGE:
		ret = purge(db, args[3:], opts)
	elif cmd == CMD_RELOAD:
		ret = reload(ser, ssl, args[3:], opts)
	elif cmd == CMD_STAT:
		ret = stat(ser, ssl, args[3:], opts)
	elif cmd == CMD_UPTIME:
		ret = uptime(ser, ssl, args[3:], opts)
	elif cmd == CMD_USER:
		ret = user(db, ser, ssl, args[3:], opts)
	elif cmd == CMD_USRLOC:
		ret = usrloc(db, ser, ssl, args[3:], opts)
	elif cmd == CMD_VERSION:
		ret = version(ser, ssl, args[3:], opts)
	elif cmd == CMD_LIST_TLS:
		ret = list_tls(ser, ssl, args[3:], opts)
	else:
		raise Error (EINVAL, cmd)
	return ret

def help(args, opts):
	return """\
Usage:
	ser_ctl [options...] [--] [command] [param...]

%s
Commands & parameters:
  - user and domain administration:
	ser_ctl alias  add  <username> <alias>
	ser_ctl alias  rm   <alias>
	ser_ctl domain add  <domain>
	ser_ctl domain rm   <domain>
	ser_ctl password    <uri> <password>
	ser_ctl user   add  <uri> <password>
	ser_ctl user   rm   <uri>
	ser_ctl usrloc add  <uri> <contact>
	ser_ctl usrloc show <uri>
	ser_ctl usrloc rm   <uri> [contact]

  - remove database records marked as deleted:
	ser_ctl purge

  - SER control, info and statistics:
	ser_ctl flush <uri>
	ser_ctl kill  [sig]
	ser_ctl ps
	ser_ctl reload
	ser_ctl stat
	ser_ctl uptime
	ser_ctl version
        ser_ctl list_tls

  - miscelaneous:
	ser_ctl publish <uid> <file_with_PIDF_doc> <expires_in_sec> [etag]
""" % serctl.ctlhelp.options(args, opts)

def purge(db, args, opts):
	for c in (Uri, Cred, Domain, User):
		o = c(db)
		o.purge()
		del(o)

def flush(ser, ssl, args, opts):
	rpc = Xml_rpc(ser, ssl)
	# FIX: TODO: what fn?
#	rpc.raw_cmd('???')
	raise Error (ENOSYS, 'flush')

def publish(ser, ssl, args, opts):
	try:
		uri = args[0]
	except:
		raise Error (ENOARG, 'uid')

	try:
		doc_file = args[1]
	except:
		raise Error (ENOARG, 'file_with_PIDF_doc')

	try:
		expires = args[2]
	except:
		raise Error (ENOARG, 'expires_in_sec')

	try:
		etag = args[3]
	except:
		etag = None

	expires = int(expires)

	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = Xml_rpc(ser, ssl)
	fh = open(doc_file)
	doc = fh.read()
	fh.close()
	if etag:
		ret = rpc.ser.pa.publish('registrar', uri, doc, expires, etag)
	else:
		ret = rpc.ser.pa.publish('registrar', uri, doc, expires)
	if astab:
		ret, desc = var2tab(ret)
		tabprint(ret, desc, rsep, lsep, astab)
	else:
		print repr(ret)


def domain(db, ser, ssl, args, opts):
	try:
		cmd_ = args[0]
	except:
		raise Error (ENOARG, '<command>')
	cmd = CMD.get(cmd_)
	try:
		dom = args[1]
	except:
		raise Error (ENOARG, '<domain>')
	if cmd == CMD_ADD:
		d = Domain_ctl(db, ser, ssl)
		d.add(dom)
	elif cmd == CMD_RM:
		d = Domain_ctl(db, ser, ssl)
		d.rm(dom)
	else:
		raise Error (EINVAL, cmd_)
		
def user(db, ser, ssl, args, opts):
	try:
		cmd_ = args[0]
	except:
		raise Error (ENOARG, '<command>')
	cmd = CMD.get(cmd_)
	try:
		uri = args[1]
	except:
		raise Error (ENOARG, '<uri>')
	if cmd == CMD_ADD:
		try:
			password = args[2]
		except:
			raise Error (ENOARG, '<password>')
		u = User_ctl(db, ser, ssl)
		u.add(uri, password)
	elif cmd == CMD_RM:
		u = User_ctl(db, ser, ssl)
		u.rm(uri)
	elif cmd == CMD_PASS:
		u = User_ctl(db, ser, ssl)
		u.passwd(uri, password)
	else:
		raise Error (EINVAL, cmd_)

def password(db, ser, ssl, args, opts):
	try:
		uri = args[0]
	except:
		raise Error (ENOARG, '<uri>')
	try:
		password = args[1]
	except:
		raise Error (ENOARG, '<password>')
	u = User_ctl(db, ser, ssl)
	u.passwd(uri, password)

def alias(db, ser, ssl, args, opts):
	try:
		cmd_ = args[0]
	except:
		raise Error (ENOARG, '<command>')
	cmd = CMD.get(cmd_)
	if cmd == CMD_ADD:
		try:
			user = args[1]
		except:
			raise Error (ENOARG, '<username>')
		try:
			alias = args[2]
		except:
			raise Error (ENOARG, '<alias>')
		a = Alias_ctl(db, ser, ssl)
		a.add(user, alias)
	elif cmd == CMD_RM:
		try:
			alias = args[1]
		except:
			raise Error (ENOARG, '<alias>')
		a = Alias_ctl(db, ser, ssl)
		a.rm(alias)
	else:
		raise Error (EINVAL, cmd_)
		
def usrloc(db, ser, ssl, args, opts):
	try:
		cmd_ = args[0]
	except:
		raise Error (ENOARG, '<command>')
	cmd = CMD.get(cmd_)
	try:
		uri = args[1]
	except:
		raise Error (ENOARG, '<uri>')
	if cmd == CMD_ADD:
		try:
			contact = args[2]
		except:
			raise Error (ENOARG, '<contact>')
		u = Usrloc_ctl(db, ser, ssl)
		u.add(uri, contact)
	elif cmd == CMD_RM:
		try:
			contact = args[1]
		except:
			contact = None
		u = Usrloc_ctl(db, ser, ssl)
		u.rm(uri, contact)
	elif cmd == CMD_SHOW:
		u = Usrloc_ctl(db, ser, ssl)
		ret = u.show(uri)
		# FIX: update
		print ret

	else:
		raise Error (EINVAL, cmd_)
		
def ps(ser, ssl, args, opts):

	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = Xml_rpc(ser, ssl)
	ret = rpc.core_ps()

	desc = [ ('id', '?', ''), ('process description', '?', '') ]
	ret = [ (str(a), b) for a, b in ret ]
	tabprint(ret, desc, rsep, lsep, astab)

def version(ser, ssl, args, opts):

	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = Xml_rpc(ser, ssl)
	ret = rpc.core_version()

	ret, desc = var2tab(ret, 'version')
	tabprint(ret, desc, rsep, lsep, astab)

def uptime(ser, ssl, args, opts):

	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = Xml_rpc(ser, ssl)
	ret = rpc.core_uptime()

	ret, desc = dict2tab(ret, ('uptime', 'up_since', 'now'))
	tabprint(ret, desc, rsep, lsep, astab)

def list_tls(ser, ssl, args, opts):
        cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = Xml_rpc(ser, ssl)
	ret = rpc.tls_list()

	desc = [ ('ID', '?', ''), ('Timeout', '?', ''), ('Source', '?', ''), ('Destination', '?', ''), ('TLS', '?', '') ]
	ret = [ (str(s['id']), str(s['timeout']), s['src_ip'] + ':' + str(s['src_port']), s['dst_ip'] + ':' + str(s['dst_port']), s['tls']) for s in ret ]
	tabprint(ret, desc, rsep, lsep, astab)

def kill(ser, ssl, args, opts):
	try:
		sig = args[0]
	except:
		sig = 15
	sig = int(sig)

	rpc = Xml_rpc(ser, ssl)
	ret = rpc.core_kill(sig)

def stat(ser, ssl, args, opts):

	cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = Xml_rpc(ser, ssl)

	# FIX: determine what exists
	uptime      = rpc.core_uptime()
	shmmem      = rpc.core_shmmem()
	tcpinfo     = rpc.core_tcp_info()
	slstats     = rpc.sl_stats()
	tmstats     = rpc.tm_stats()
	usrlocstats = rpc.usrloc_stats()

	ret, desc = dict2tab(uptime, ('uptime', 'up_since', 'now'))
	tabprint(ret, desc, rsep, lsep, astab)

	ret, desc = dict2tab(shmmem)
	tabprint(ret, desc, rsep, lsep, astab)

	ret, desc = dict2tab(tcpinfo)
	tabprint(ret, desc, rsep, lsep, astab)

	ret, desc = dict2tab(slstats)
	tabprint(ret, desc, rsep, lsep, astab)

	ret, desc = dict2tab(tmstats)
	tabprint(ret, desc, rsep, lsep, astab)

	ret, desc = var2tab(usrlocstats)
	tabprint(ret, desc, rsep, lsep, astab)

	# FIX: page redesign

#
# janakj: FIXME: Is is necessary to run system.listMethods
# and test for function availability ? Wouldn't just trying
# to execute a function and catching corresponding exception
# be enough ?
#
def reload(ser, ssl, args, opts):

	rpc = Xml_rpc(ser, ssl)

	exist = rpc.ser.system.listMethods()

	if 'domain.reload' in exist:
		ret = rpc.ser.domain.reload()

	if 'tls.reload' in exist:
		ret = rpc.ser.tls.reload()

	# FIX: what more?

class Domain_ctl:
	def __init__(self, dburi, seruri, ssl=None):
		self.db  = dburi
		self.ser = seruri
		self.ssl = ssl

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
		rpc = Xml_rpc(self.ser, self.ssl)
		try:
			rpc.ser.domain.reload()
		except xmlrpclib.Fault, inst:
			warning('Domain reloading fail: ' +  repr(inst))

class User_ctl:
	def __init__(self, dburi, seruri, ssl=None):
		self.db  = dburi
		self.ser = seruri
		self.ssl = ssl

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
	def __init__(self, dburi, seruri, ssl=None):
		self.db  = dburi
		self.ser = seruri
		self.ssl = ssl

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
	def __init__(self, dburi, seruri, ssl=None):
		self.db  = dburi
		self.ser = seruri
		self.ssl = ssl

	def _get_uid(self, uri):
		ur = Uri(self.db)
		uids = ur.get_uids(uri, flags=[IS_FROM])
		if not uids:
			raise Error (ENOUSER, 'For uri=%s (with TO_FLAG set)' % uri)
		return uids[0]

	def show(self, uri):
		uid = self._get_uid(uri)
		rpc = Xml_rpc(self.ser, self.ssl)
		return rpc.ser.usrloc.show_contacts('location', uid)

	def add(self, uri, contact):
		uid = self._get_uid(uri)
		rpc = Xml_rpc(self.ser, self.ssl)
		rpc.ser.usrloc.add_contact('location', uid, contact, 0, 1, 128)

	def rm(self, uri, contact=None):
		uid = self._get_uid(uri)
		rpc = Xml_rpc(self.ser, self.ssl)
		if contact is None:
			rpc.ser.usrloc.delete_uid('location', uid)
		else:
			rpc.ser.usrloc.delete_contact('location', uid, contact)
