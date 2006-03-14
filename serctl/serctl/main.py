#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: main.py,v 1.12 2006/03/14 09:11:10 hallik Exp $
#
# Copyright (C) 2005 FhG iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
try:
	from getopt         import gnu_getopt
except:
	from serctl.backports.getopt import gnu_getopt
from serctl.error   import set_excepthook, Error, EINVAL, ENODB, ENOSYS, \
                           ENOSER, ENOENT, EICONF, EINAME, ENOHELP, ENOARG, \
                           EEXTRA
from serctl.uri     import adjust_ser_uri
from os.path        import basename
import sys, os, os.path
import serctl.options as opt
import serctl.config as config


def parse_cmdline(argv):
	lineopts, args = gnu_getopt(argv, opt.GETOPT_SHORT, opt.GETOPT_LONG)
	opts = {}
	for o, desc in opt.OPT.items():
		if len(desc) < 4: continue
		opts[o] = desc[3]
	for o, v in lineopts:
		o = opt.OPTS[o]
		if opt.OPT[o][2] is False:
			v = True
		opts[o] = v
	return args, opts

def handle_config(path = None):
	if path is None:
		path = config.CONFIG
		if not os.path.exists(path):
			return {}
	elif not os.path.exists(path):
		raise Error (ENOENT, path)
	l = {}
	execfile(path, {}, l)
	for k in l.keys():
		if not opt.OPT.has_key(k):
			raise Error (EICONF, k)
	return l

def handle_db_uri(opts):
	if opts['DB_URI'] is not None:
		return opts['DB_URI']
	return os.getenv(opts['ENV_DB'], config.DB_URI)

def handle_ser_uri(opts):
	if opts['SER_URI'] is not None:
		return adjust_ser_uri(opts['SER_URI'])
	uri = os.getenv(opts['ENV_SER'], config.SER_URI)
	return adjust_ser_uri(uri)

def handle_module_name(cmd_name):
	bname = basename(cmd_name)
	name = bname.lower().replace('-', '_')
	if name[:4] == "ser_":
		return name[4:]
	else:
		raise Error (EINAME, bname)

def module(name):
	name = 'ctl' + name
	pkgname = 'serctl.' + name
	mod = __import__(pkgname)
	mod = getattr(mod, name)
	return mod

def call(func, args, opts):
	vars = func.func_code.co_varnames[:func.func_code.co_argcount]
	n = len(vars)
	x = len(args)
	if func.func_defaults is None:
		m = 0
	else:
		m = len(func.func_defaults)
	if x < (n - m):
		raise Error (ENOARG, vars[x])
	if not (func.func_code.co_flags & 0x04) and x > n:
		raise Error (EEXTRA, args[n:])
	if func.func_code.co_flags & 0x08:
		return apply(func, args, opts)
	return apply(func, args)

def main(argv):
	args, opts = parse_cmdline(argv)
	set_excepthook(opts['DEBUG'])
	new_opts = handle_config(opts['CONFIG'])
	opts.update(new_opts)
	set_excepthook(opts['DEBUG'])
	if len(args) < 1:
		raise Error (EINVAL)
	opts['DB_URI']  = handle_db_uri(opts)
	opts['SER_URI'] = handle_ser_uri(opts)

	modname = handle_module_name(args[0])
	mod = module(modname)

	errarg = None
	try:
		funcname = mod._handler_
		args = args[1:]
	except:
		try:
			errarg = args[1]
			funcname = opt.CMD.get(args[1])
		except IndexError:
			funcname = 'help'
		if opts['HELP']:
			funcname = 'help'
		args = args[2:]

	try:
		func = getattr(mod, funcname)
	except:
		if errarg:
			raise Error (EINVAL, errarg)
		else:
			raise Error (EINVAL,)

	return call(func, args, opts)
