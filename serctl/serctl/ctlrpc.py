#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlrpc.py,v 1.2 2006/02/15 18:51:28 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.error     import Error, ENOARG, ENOSYS
from serctl.serxmlrpc import ServerProxy
from serctl.options   import OPT_SSL_KEY, OPT_SSL_CERT, OPT_SER_URI, OPT_FIFO
from serctl.utils     import show_opts, tabprint
import serctl.ctlhelp

def main(args, opts):
	if len(args) < 3:
		print help(args, opts)
		return

	if opts.has_key(OPT_FIFO):
		ret = fifo_rpc(args[2:], opts)
	else:
		ret = xml_rpc(args[2:], opts)
        return ret

def _rpc2tab(data):
	if type(data) == dict:
		ret  = [ (str(k), str(v)) for k, v in data.items() ]
		desc = [ ('key', '?', ''), ('value', '?', '') ]
	elif type(data) == tuple or type(data) == list:
		ret  = [ (str(i), ) for i in data ]
		desc = [ ('value', '?', ''), ]
	else:
		ret  = [ (str(data), ) ] 
		desc = [ ('value', '?', ''), ]
	return ret, desc

def xml_rpc(args, opts):
	if len(args) < 1:
		raise Error, (ENOARG, 'rpc_command')

	ssl_key  = opts[OPT_SSL_KEY]
	ssl_cert = opts[OPT_SSL_CERT]
	ser_uri  = opts[OPT_SER_URI]

        cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = Xml_rpc(ser_uri, (ssl_key, ssl_cert))

	cmd    = args[0]
	params = args[1:]

	ret = rpc.raw_cmd(cmd, params)
	if astab:
		ret, desc = _rpc2tab(ret)
	        tabprint(ret, desc, rsep, lsep, astab)
	else:
		print repr(ret)
	
def fifo_rpc(args, opts):
	if len(args) < 1:
		raise Error, (ENOARG, 'rpc_command')

	rpc = Fifo_rpc()

	cmd    = args[0]
	params = args[1:]

	print rpc.raw_cmd(cmd, params)
	

def help(args, opts):
	return """\
Usage:
	ser_rpc [options...] [--] <rpc_command> [rpc_params...]
%s
""" % serctl.ctlhelp.options(args, opts)

class Xml_rpc:
	def __init__(self, ser_uri, ssl=None):
		self.ser = ServerProxy(ser_uri, ssl)

	def raw_cmd(self, cmd, params=[]):
		cmd = 'self.ser.' + cmd
		params = ', '.join(params)
		cmd = cmd + '(' + params + ')'
		return eval(cmd)

class Fifo_rpc:
	def __init__(self):
		pass

	def raw_cmd(self, cmd, params=[]):
		raise Error, (ENOSYS, 'fifo rpc call')
