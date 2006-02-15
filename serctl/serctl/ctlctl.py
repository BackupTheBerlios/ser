#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlctl.py,v 1.7 2006/02/15 18:51:28 hallik Exp $
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
from serctl.error     import Error, ENOARG, EINVAL, ENOSYS
from serctl.options   import CMD_FLUSH, CMD_PURGE, OPT_DATABASE, CMD_PUBLISH, \
                             OPT_SER_URI, CMD, CMD_HELP
from serctl.serxmlrpc import ServerProxy
from serctl.utils     import show_opts
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

	if   cmd == CMD_FLUSH:
		ret = flush(db, args[3:], opts)
	elif cmd == CMD_PUBLISH:
		ret = publish(db, args[3:], opts)
	elif cmd == CMD_PURGE:
		ret = purge(db, args[3:], opts)
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
	ser_ctl flush   <uri>
	ser_ctl publish <uri> <file_with_PIDF_doc> <expires_in_sec> [etag]
	ser_ctl purge
""" % serctl.ctlhelp.options(args, opts)

def purge(db, args, opts):
	for c in (Uri, Cred, Domain, User):
		o = c(db)
		o.purge()
		del(o)

def flush(db, args, opts):

	ser = ServerProxy(opts[OPT_SER_URI])

	# FIX: TODO: what fn?
	# ser.fn(...)
	raise Error (ENOSYS, 'flush')

def publish(db, args, opts):
	try:
		uri = args[0]
	except:
		raise Error (ENOARG, 'uri')

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

	ser = ServerProxy(opts[OPT_SER_URI])
	fh = open(doc_file)
	doc = fh.read()
	fh.close()
	if etag:
		ret = ser.pa.publish('registrar', uri, doc, expires, etag)
	else:
		ret = ser.pa.publish('registrar', uri, doc, expires)

	print ret

