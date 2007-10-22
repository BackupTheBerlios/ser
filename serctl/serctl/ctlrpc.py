#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlrpc.py,v 1.16 2007/10/22 22:58:29 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

_handler_ = 'main'

from os.path          import join, dirname
from serctl.error     import Error, ENOSYS, ERPC, warning
from serctl.utils     import show_opts, tabprint, var2tab
import os, sys, serctl.ctlhelp, xmlrpclib, serctl.serxmlrpc, version, serctl.binrpc

serctl.serxmlrpc.Transport.HEADERS['User-Agent'] = 'serctl/' + version.version
ServerProxy = serctl.serxmlrpc.ServerProxy

def main(rpc_command=None, *args, **opts):
	if opts['HELP'] or rpc_command==None:
		return help()
        return rpc(rpc_command, args, opts)

def help(*tmp):
	print """\
Usage:
	ser_rpc [options...] [--] <rpc_command> [rpc_params...]

%s
""" % serctl.ctlhelp.options()

def any_rpc(opts):
	if opts['USE_FIFO']:
		ser_fifo = opts['FIFO']
		return Fifo_rpc(ser_fifo)
	ssl_key  = opts['SSL_KEY']
	ssl_cert = opts['SSL_CERT']
	ser_uri  = opts['SER_URI']

	if ser_uri[:4] == 'unix':
		return Bin_rpc(ser_uri)
	return Xml_rpc(ser_uri, (ssl_key, ssl_cert))

def multi_rpc(opts):
	ssl_key  = opts['SSL_KEY']
	ssl_cert = opts['SSL_CERT']
	servers  = opts['SERVERS']

	return Multi_rpc(servers, (ssl_key, ssl_cert))

def rpc(cmd, args, opts):

	rpc = any_rpc(opts)

        cols, numeric, limit, rsep, lsep, astab = show_opts(opts)

	# quote unquoted string that not appear as number
	args = list(args)
	for i in range(len(args)):
		try:
			tmp = float(args[i])
		except:
			if args[i][:1] not in ('"', "'"):
				args[i] = '"' + str(args[i]) + '"'

	ret = rpc.shell_cmd(cmd, args)
	if astab:
		ret, desc = var2tab(ret)
	        tabprint(ret, desc, rsep, lsep, astab)
	else:
		if ret == '':      # ignore empty string it's default
			return     # value if nothing returns
		print repr(ret)
	

class Multi_rpc:
	def __init__(self, servers, ssl=None):
		self.ser = _ser(self.raw_cmd)
		self.servers = servers
		self.ssl = ssl

	def _wnote(self, warn):
		if warn:
			print "For the change to take effect on all machines in the cluster once available, execute the following command: 'ser_ctl reload'."

	def raw_cmd(self, cmd, par=[]):
		warn = []
		for server in self.servers:
			ser = ServerProxy(server, self.ssl)
			method = getattr(ser, cmd)
			try:
				apply(method, par)
			except xmlrpclib.Fault, inst:
				warning("RPC on '%s' failed: %s" %  (server, str(inst)))
				warn.append(server)
			except:
				warning("RPC on '%s' failed: %s" % (server, str(sys.exc_info()[0])))
				warn.append(server)
		self._wnote(warn)

	def cmd(self, cmd, *par):
		return self.raw_cmd(cmd, par)

	def cmd_if_exist(self, cmd, par=[]):
		warn = []
		for server in self.servers:
			try:
				ser = ServerProxy(server, self.ssl)
				if cmd not in ser.system.listMethods(): continue
			except xmlrpclib.Fault, inst:
				warning("ListMethods on '%s' failed: %s" %  (server, str(inst)))
				warn.append(server)
				continue
			except:
				warning("ListMethods on '%s' failed: %s" % (server, str(sys.exc_info()[0])))
				warn.append(server)
				continue
			method = getattr(ser, cmd)
			try:
				apply(method, par)
			except xmlrpclib.Fault, inst:
				warn.append(server)
				warning("RPC on '%s' failed: %s" %  (server, str(inst)))
			except:
				warn.append(server)
				warning("RPC on '%s' failed: %s" % (server, str(sys.exc_info()[0])))
		self._wnote(warn)

	def reload(self):
		warn = []
		for server in self.servers:
			try:
				ser = ServerProxy(server, self.ssl)
				cmds = [ i for i in ser.system.listMethods() if i[-7:] == '.reload' ]
			except xmlrpclib.Fault, inst:
				warning("ListMethods on '%s' failed: %s" %  (server, str(inst)))
				warn.append(server)
				continue
			except:
				warning("ListMethods on '%s' failed: %s" % (server, str(sys.exc_info()[0])))
				warn.append(server)
				continue
			for cmd in cmds:
				method = getattr(ser, cmd)
				try:
					apply(method)
				except xmlrpclib.Fault, inst:
					warn.append(server)
					warning("Reload on '%s' failed: %s" %  (server, str(inst)))
				except:
					warning("Reload on '%s' failed: %s" % (server, str(sys.exc_info()[0])))
					warn.append(server)

		self._wnote(warn)

class Xml_rpc:
	def __init__(self, ser_uri, ssl=None):
		self.ser = ServerProxy(ser_uri, ssl)

	def raw_cmd(self, cmd, par=[]):
		method = getattr(self.ser, cmd)
		return apply(method, par)

	def shell_cmd(self, cmd, par=[]):
		params = [ eval(str(i)) for i in par ]
		return self.raw_cmd(str(cmd), params)

	def cmd(self, cmd, *par):
		return self.raw_cmd(cmd, par)


	def reload(self):
		try:
			exists = [ i for i in self.ser.system.listMethods() if i[-7:] == '.reload' ]
		except xmlrpclib.Fault, inst:
			warning("ListMethods failed: %s" %  str(inst))
			return
		except:
			warning("ListMethods failed: %s" % str(sys.exc_info()[0]))
			return
		for fn in exists:
			try:
				self.raw_cmd(fn)
			except xmlrpclib.Fault, inst:
				warning('Reloading fail: ' +  str(inst))
			except Error, inst:
				warning('Reloading fail: ' + repr(inst))
			except:
				warning('Reloading fail: ' + str(sys.exc_info()[0]))

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

class _ser:

	def __init__(self, fn):
		self._fn = fn

	def __getattr__(self, what):
		i = _ser(self._fn)
		if self.__dict__.has_key('_name'):
			i._name = self.__dict__['_name'] + '.' + what
		else:
			i._name = what
		return i

	def __call__(self, *params):
		return self._fn(self._name, params)


class Fifo_rpc(Xml_rpc):

	LIST = (\
		'core.ps',
	)

	def __init__(self, fifo='/tmp/ser_fifo'):
		self.fifo = fifo
		self.ser = _ser(self.raw_cmd)

	def _escape(self, s):
		s = s.replace('\n', '\\n')
		s = s.replace('\r', '\\r')
		s = s.replace('\t', '\\t')
		s = s.replace('\\', '\\\\')
		s = s.replace('\0', '\\0')
		s = s.replace(':',  '\\o')
		s = s.replace(',',  '\\c')
		return s

	def _unescape(self, s):
		s = s.replace('\\n',  '\n')
		s = s.replace('\\r',  '\r')
		s = s.replace('\\t',  '\t')
		s = s.replace('\\\\', '\\')
		s = s.replace('\\0',  '\0')
		s = s.replace('\\o',  ':')
		s = s.replace('\\c',  ',')
		return s

	def _mkfifo(self):
		name = 'ser_receiver_' +  str(os.getpid())
		fifo = join('/tmp', name)
		os.mkfifo(fifo)
		os.chmod(fifo, 0666)
		return name, fifo

	def _read_reply(self, fifo):
		fh = open(fifo, 'r')
		reply = fh.read()
		fh.close()
		return reply

	def raw_cmd(self, cmdname, params):
		fname, rfifo = self._mkfifo()
		cmd = ':' + cmdname + ':' + fname + '\n'

		for p in params:
			cmd += self._escape(self._escape(str(p))) + '\n'
		cmd += '\n'
		if os.fork() != 0:
			fh = open(self.fifo, 'w')
			fh.write(cmd)
			fh.close()
			ret = os.wait()
			os.unlink(rfifo)
			sys.exit()
		reply = self._read_reply(rfifo)
		st, reply = reply.split('\n', 1)
		if st[:3] != '200':
			raise Error (ERPC, st)
		reply = reply.strip()
		if cmdname not in self.LIST and reply.find(':') > 0:
			ret = {}
			for r in reply.split(','):
				k, v = r.split(':', 1)
				ret[self._unescape(k)] = self._unescape(v).lstrip()
		else:
			ret = []
			for r in reply.split('\n'):
				ret.append(self._unescape(r))
		return ret

class Bin_rpc(Xml_rpc):

	def __init__(self, uri="unixs:/tmp/ser_ctl"):
		self.ser = _ser(self.raw_cmd)
		u = uri.lstrip("unix")
		dgram = False
		if u[0] == ':':
			u = u.lstrip(':')
		elif u[0] == 's':
			u = u.lstrip('s:')
		else:
			u = u.lstrip('d:')
			dgram = True
		if dgram:
			self.sck = serctl.binrpc.mksockud(u)
		else:
			self.sck = serctl.binrpc.mksockus(u)

	def raw_cmd(self, cmdname, params):
		ret = serctl.binrpc.rpc(self.sck, cmdname, params)
		return ret
