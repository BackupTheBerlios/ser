#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlrpc.py,v 1.7 2006/03/08 23:27:52 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

_handler_ = 'main'

from serctl.error     import Error, ENOSYS
from serctl.serxmlrpc import ServerProxy
from serctl.utils     import show_opts, tabprint, var2tab
import serctl.ctlhelp

def main(rpc_command=None, *args, **opts):
	if opts['HELP'] or rpc_command==None:
		return help()
	if opts['FIFO']:
		ret = fifo_rpc(rpc_command, args, opts)
	else:
		ret = xml_rpc(rpc_command, args, opts)
        return ret

def help(*tmp):
	print """\
Usage:
	ser_rpc [options...] [--] <rpc_command> [rpc_params...]

%s
""" % serctl.ctlhelp.options()

def xml_rpc(cmd, args, opts):
	ssl_key  = opts['SSL_KEY']
	ssl_cert = opts['SSL_CERT']
	ser_uri  = opts['SER_URI']

        cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	rpc = Xml_rpc(ser_uri, (ssl_key, ssl_cert))

	ret = rpc.shell_cmd(cmd, args)
	if astab:
		ret, desc = var2tab(ret)
	        tabprint(ret, desc, rsep, lsep, astab)
	else:
		print repr(ret)
	
def fifo_rpc(cmd, args, opts):
	rpc = Fifo_rpc()

	# FIX: todo
	print rpc.shell_cmd(cmd, args)
	


class Xml_rpc:
	def __init__(self, ser_uri, ssl=None):
		self.ser = ServerProxy(ser_uri, ssl)

	def raw_cmd(self, cmd, par=[]):
		cmd = 'self.ser.' + str(cmd)
		params = [ 'par[' + str(i) + ']' for i in range(len(par)) ]
		params = ', '.join(params)
		cmd = cmd + '(' + params + ')'
		return eval(cmd)

	def shell_cmd(self, cmd, par=[]):
		cmd = 'self.ser.' + str(cmd)
		params = [ str(i) for i in par ]
		params = ', '.join(params)
		cmd = cmd + '(' + params + ')'
		return eval(cmd)

	def cmd(self, cmd, *par):
		return self.raw_cmd(cmd, par)

	def core_ps(self):
		ps = self.ser.core.ps()
		l = []
		for i in range(0, len(ps), 2):
			l.append((ps[i], ps[i+1]))
		return l

	def core_version(self):
		ver = self.ser.core.version()
		return ver

	def core_uptime(self):
		uptime = self.ser.core.uptime()
		for k in uptime.keys():
			uptime[k] = str(uptime[k]).strip()
		uptime['uptime'] = float(uptime['uptime'])
		return uptime

	def core_kill(self, sig=15):
		self.ser.core.kill(sig)

	def core_shmmem(self):
		return self.ser.core.shmmem()

	def core_tcp_info(self):
		return self.ser.core.tcp_info()

	def sl_stats(self):
		return self.ser.sl.stats()

	def tm_stats(self):
		return self.ser.tm.stats()

	def usrloc_stats(self):
		return self.ser.usrloc.stats()

	def tls_list(self):
		return self.ser.tls.list()

	def system_listmethods(self):
		return self.ser.system.listMethods()

class Fifo_rpc:
	def __init__(self):
		pass

	# FIX: todo
	def shell_cmd(self, cmd, par=[]):
		raise Error (ENOSYS, 'fifo-rpc-call')
