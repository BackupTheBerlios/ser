#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlctl.py,v 1.5 2006/01/16 17:35:14 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from ctluri       import Uri
from ctlcred      import Cred
from ctldomain    import Domain
from ctluser      import User
from error        import Error, ENOARG, EINVAL, ENOSYS
from options      import CMD_FLUSH, CMD_PURGE, OPT_DATABASE, CMD_PUBLISH, \
                         OPT_SER_URI
from serxmlrpc    import ServerProxy
from utils        import show_opts
import ctlhelp

def main(args, opts):
	cmd = args[2]
	db  = opts[OPT_DATABASE]

	if   cmd == CMD_FLUSH:
		ret = flush(db, args[3:], opts)
	elif cmd == CMD_PUBLISH:
		ret = publish(db, args[3:], opts)
	elif cmd == CMD_PURGE:
		ret = purge(db, args[3:], opts)
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
""" % ctlhelp.options(args, opts)

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
