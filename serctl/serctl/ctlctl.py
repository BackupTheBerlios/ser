#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlctl.py,v 1.2 2005/12/27 10:13:04 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# Created:     2005/12/16
# Last update: 2005/12/27

from ctluri    import Uri
from ctlcred   import Cred
from ctldomain import Domain
from ctluser   import User
from error     import Error, ENOARG, EINVAL, ENOSYS
from options   import CMD_FLUSH, CMD_PURGE, OPT_DATABASE
from xmlrpclib import ServerProxy

def main(args, opts):
	cmd = args[2]
	db  = opts[OPT_DATABASE]

	if   cmd == CMD_FLUSH:
		ret = flush(db, args[3:], opts)
	elif cmd == CMD_PURGE:
		ret = purge(db, args[3:], opts)
	else:
		raise Error (EINVAL, cmd)
	return ret

def help(args, opts):
	return """\
Usage:
	ser_ctl [options...] [--] [command] [param...]

Commands & parameters:
	ser_ctl flush <uri>
	ser_ctl purge
"""

def purge(db, args, opts):
	for c in (Uri, Cred, Domain, User):
		o = c(db)
		o.purge()
		del(o)

def flush(db, args, opts):
	try:
		uri = args[0]
	except:
		raise Error (ENOARG, 'uri')

	server = ServerProxy(uri)

	# FIX: TODO: what fn?
	#server.fn(...)
	raise Error (ENOSYS, 'flush')
