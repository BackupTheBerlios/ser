#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlrpc.py,v 1.1 2006/01/18 17:49:20 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serxmlrpc import ServerProxy
from options   import OPT_SSL_KEY, OPT_SSL_CERT, OPT_SER_URI
import ctlhelp

def main(args, opts):
	if len(args) < 3:
		print help(args, opts)
		return

	ssl_key  = opts[OPT_SSL_KEY]
	ssl_cert = opts[OPT_SSL_CERT]
	ser      = opts[OPT_SER_URI]

	ser = ServerProxy(ser, (ssl_key, ssl_cert))

	cmd = 'ser.' + args[2]
	params = ', '.join(args[3:])

	cmd = cmd + '(' + params + ')'

	print eval(cmd)



def help(args, opts):
	return """\
Usage:
	ser_rpc <xmlrpc_command> [xmlrpc_params...]
"""
