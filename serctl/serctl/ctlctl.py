#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlctl.py,v 1.9 2006/02/21 21:34:27 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.ctluri    import Uri
from serctl.ctlcred   import Cred
from serctl.ctldomain import Domain
from serctl.ctluser   import User
from serctl.error     import Error, ENOARG, EINVAL, ENOSYS, EDOMAIN, ENODOMAIN
from serctl.options   import CMD_FLUSH, CMD_PURGE, OPT_DATABASE, CMD_PUBLISH, \
                             OPT_SER_URI, CMD, CMD_HELP, CMD_DOMAIN, CMD, \
                             CMD_ADD, CMD_RM, OPT_SSL_KEY, OPT_SSL_CERT, \
                             CMD_USER, CMD_PASS, CMD_ALIAS
from serctl.ctlrpc    import Xml_rpc
from serctl.uri       import split_sip_uri
from serctl.utils     import show_opts, var2tab, tabprint
import serctl.ctlhelp

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
	elif cmd == CMD_PASS:
		ret = password(db, ser, ssl, args[3:], opts)
	elif cmd == CMD_PUBLISH:
		ret = publish(ser, ssl, args[3:], opts)
	elif cmd == CMD_PURGE:
		ret = purge(db, args[3:], opts)
	elif cmd == CMD_USER:
		ret = user(db, ser, ssl, args[3:], opts)
	elif cmd == CMD_HELP:
		print help(args, opts)
		return
	else:
		raise Error (EINVAL, cmd)
	return ret

def help(args, opts):
	return """\
Usage:
	ser_ctl [options...] [--] [command] [param...]

%s
Commands & parameters:
  - flush the DB caches:
	ser_ctl flush   <uri>

  - user and domain administration:
	ser_ctl alias  add <username> <alias>
	ser_ctl alias  rm  <alias>
	ser_ctl domain add <domain>
	ser_ctl domain rm  <domain>
	ser_ctl password   <uri> <password>
	ser_ctl user   add <uri> <password>
	ser_ctl user   rm  <uri>

  - remove records marked as deleted from DB:
	ser_ctl purge

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
		rpc.ser.domain.reload()

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
		ur.add(uri, username)

	def rm(self, alias):
		ur = Uri(self.db)
		uid, did = ur.get_alias_info(alias, canonical=False)
		uri = alias + '@' + did
		ur.rm(uri=uri, uid=uid, did=did)
		ur.purge()
