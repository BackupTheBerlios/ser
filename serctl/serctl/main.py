#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: main.py,v 1.5 2006/01/12 20:27:06 hallik Exp $
#
# Copyright (C) 2005 FhG iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

import config

from error   import Error, EINVAL, ENODB, ENOSYS, ENOSER, set_excepthook
from getopt  import gnu_getopt
from options import *
from os.path import basename
import sys, os

def parse_opts(cmd_line):
	lineopts, args = gnu_getopt(cmd_line, GETOPT_SHORT, GETOPT_LONG)
	opts = {}
	for o, v in lineopts:
		o = OPTS[o]
		opts[o] = v
		if   o == OPT_VERBOSE:
			config.VERB += 1
		elif o == OPT_QUIET:
			config.VERB = 0
		elif o == OPT_DEBUG:
			config.DEBUG = not config.DEBUG
			set_excepthook()
	return args, opts

def module(name):
	if not OBJ.has_key(name):
		return None
	name = OBJ[name]
	if name == OBJ_CTL:
		import ctlctl
		return ctlctl
	if name == OBJ_ATTR:
		import ctlattr
		return ctlattr
	if name == OBJ_CREDENTIAL:
		import ctlcred
		return ctlcred
	if name == OBJ_DOMAIN:
		import ctldomain
		return ctldomain
	if name == OBJ_HELP:
		import ctlhelp
		return ctlhelp
	if name == OBJ_RAW:
		import ctlraw
		return ctlraw
	if name == OBJ_USER:
		import ctluser
		return ctluser
	if name == OBJ_URI:
		import ctluri
		return ctluri

	# intercept module not in public release.
	if name == OBJ_INTERCEPT:
		try:
			import ctlintercept
		except:
			raise Error (ENOSYS, 'Interception control')
		return ctlintercept

	return None

def handle_help(args, opts):
	if not (args[1] == OBJ_HELP or \
	        args[2] == CMD_HELP or \
	        opts.has_key(OPT_HELP)):
		return
	mod = module(args[1])
	try:
		print mod.help(args, opts)
	except:
		mod = module(OBJ_HELP)
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

def handle_cmdname(args):
	if not args:
		return
	name = basename(args[0])
	name = name.lower().replace('_', '-')
	if name.find('-') == -1:
		return
	obj = name.split('-')[1]
	args.insert(1, obj)

def main(argv):
	args, opts = parse_opts(argv)

	handle_cmdname(args)

	if len(args) < 1:
		raise Error (EINVAL)
	if len(args) < 2:
		args.append(OBJ_HELP)
	if len(args) < 3:
		args.append(CMD_HELP)
	try:
		args[1] = OBJ[args[1]]
	except KeyError:
		raise Error (EINVAL, args[1])
	try:
		args[2] = CMD[args[2]]
	except KeyError:
		raise Error (EINVAL, args[2])

	if OPT_HELP in opts.keys():
		args[2] = CMD_HELP

	handle_help(args, opts)
	handle_db(opts)
	handle_ser_uri(opts)

	mod = module(args[1])
	if mod:
		return mod.main(args, opts)
	else:
		raise Error (EINVAL, str(args[1]))
