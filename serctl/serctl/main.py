#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: main.py,v 1.8 2006/02/15 18:51:29 hallik Exp $
#
# Copyright (C) 2005 FhG iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from getopt         import gnu_getopt
from serctl.config  import config
from serctl.error   import Error, EINVAL, ENODB, ENOSYS, ENOSER, set_excepthook
from serctl.options import *
from os.path        import basename
import sys, os, os.path

def parse_opts(cmd_line):
	lineopts, args = gnu_getopt(cmd_line, GETOPT_SHORT, GETOPT_LONG)
	opts = {}
	for o, v in lineopts:
		o = OPTS[o]
		opts[o] = v
	return args, opts

def module(name):
	if not MOD.has_key(name):
		return None
	name = MOD[name]
	if name == MOD_CTL:
		import serctl.ctlctl
		return serctl.ctlctl
	if name == MOD_ATTR:
		import serctl.ctlattr
		return serctl.ctlattr
	if name == MOD_CREDENTIAL:
		import serctl.ctlcred
		return serctl.ctlcred
	if name == MOD_DB:
		import serctl.ctldb
		return serctl.ctldb
	if name == MOD_DOMAIN:
		import serctl.ctldomain
		return serctl.ctldomain
	if name == MOD_HELP:
		import serctl.ctlhelp
		return serctl.ctlhelp
	if name == MOD_RPC:
		import serctl.ctlrpc
		return serctl.ctlrpc
	if name == MOD_USER:
		import serctl.ctluser
		return serctl.ctluser
	if name == MOD_URI:
		import serctl.ctluri
		return serctl.ctluri

	# intercept module not in public release.
	if name == MOD_INTERCEPT:
		try:
			import serctl.ctlintercept
		except:
			raise Error (ENOSYS, 'Interception control')
		return serctl.ctlintercept

	return None

def handle_config(path = None):
	if path is None:
		path = config.CONFIG
	if path is None or not os.path.exists(path):
		return
	l = {}
	execfile(path, {}, l)
	for k, v in l.items():
		config[k] = v

def handle_debug(opts):
	if opts.has_key(OPT_DEBUG):
		config.DEBUG = not config.DEBUG
	set_excepthook(config.DEBUG)

def handle_cmdname(args):
	if not args:
		return
	name = basename(args[0])
	name = name.lower().replace('_', '-')
	if name.find('-') == -1:
		return
	mod = name.split('-')[1]
	args.insert(1, mod)

def handle_help(args, opts):
	if not (args[1] == MOD_HELP or \
	        opts.has_key(OPT_HELP)):
		return
	mod = module(args[1])
	try:
		print mod.help(args, opts)
	except:
		mod = module(MOD_HELP)
		print mod.help(args, opts)
	sys.exit(0)


def handle_db(opts):
	if opts.has_key(OPT_DATABASE):
		return
	db = os.getenv(config.ENV_DB)
	if db:
		opts[OPT_DATABASE] = db
		return
	try:
		opts[OPT_DATABASE] = config.DB_URI
	except:
		raise Error (ENODB)

def handle_ser_uri(opts):
	if opts.has_key(OPT_SER_URI):
		return
	db = os.getenv(config.ENV_SER)
	if db:
		opts[OPT_SER_URI] = db
		return
	try:
		opts[OPT_SER_URI] = config.SER_URI
	except:
		raise Error (ENOSER)

def handle_ssl_key(opts):
	if opts.has_key(OPT_SSL_KEY):
		return
	key = os.getenv(config.ENV_SSL_KEY)
	if key:
		opts[OPT_SSL_KEY] = key
		return
	try:
		opts[OPT_SSL_KEY] = config.SSL_KEY
	except:
		pass

def handle_ssl_cert(opts):
	if opts.has_key(OPT_SSL_CERT):
		return
	cert = os.getenv(config.ENV_SSL_CERT)
	if cert:
		opts[OPT_SSL_CERT] = cert
		return
	try:
		opts[OPT_SSL_CERT] = config.SSL_CERT
	except:
		pass


def main(argv):
	args, opts = parse_opts(argv)

	handle_config(opts.get(OPT_CONFIG))
	handle_debug(opts)
	handle_cmdname(args)

	if len(args) < 1:
		raise Error (EINVAL)
	if len(args) < 2:
		args.append(MOD_HELP)
	try:
		args[1] = MOD[args[1]]
	except KeyError:
		raise Error (EINVAL, args[1])

	handle_help(args, opts)
	handle_db(opts)
	handle_ser_uri(opts)
	handle_ssl_key(opts)
	handle_ssl_cert(opts)

	mod = module(args[1])
	if mod:
		return mod.main(args, opts)
	else:
		raise Error (EINVAL, str(args[1]))
