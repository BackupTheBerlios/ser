#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlhelp.py,v 1.9 2006/03/30 22:04:52 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.options import OPT
import serctl, glob, os.path

OPT_DESC = {\
	'AS_TABLE' : 'Show output as table',
	'COLUMNS'  : 'Show only specified columns (comma separated)',
	'CONFIG'   : 'Path to config file',
	'DB_URI'   : 'Database URI',
	'DBG_ARGS' : 'Show arguments and options',
	'DEBUG'    : 'Switch on python backtrace listing',
	'DISABLE'  : 'Enable',
	'ENABLE'   : 'Disable',
	'ENV_DB'   : 'Env var used to pass database uri', 
	'ENV_SER'  : 'Env var used to pass ser uri', 
	'FIFO'     : 'Path to fifo',
	'FLAGS'    : 'Numeric or symbolic flags',
	'FORCE'    : 'Ignore non-fatal errors',
	'HELP'     : 'This text',
	'LIMIT'    : 'Show only limited number of records',
	'LINE_SEP' : 'Line separator (show command)',
	'NUMERIC'  : 'Use numeric (raw) - not symbolic show',
	'PASSWORD' : 'Password',
	'REC_SEP'  : 'Record separator (show command)',
	'SER_URI'  : 'Ser uri for xml-rpc operations',
	'SSL_CERT' : 'Path to ssl cert file',
	'SSL_KEY'  : 'Path to ssl key file',
	'USE_FIFO' : 'Use fifo instead xml-rpc',
	'VERSION'  : 'Show version and exit',
}

def _gt(a, b):
	if len(a) > len(b): return a
	return b

def options():
	x = {}
	for k, v in OPT.items():
		x[v[0]] = k
	keys = x.keys()
	keys.sort()
	keys = [ x[k] for k in keys ]
	longs = [ OPT[k][1] for k in keys ]
	n = len(reduce(_gt, longs)) + 1
	opts = []
	for k in keys:
		desc = OPT_DESC.get(k, '')
		o = '\t-' + OPT[k][0] + ', --' + OPT[k][1].ljust(n) + ': ' + desc
		opts.append(o)

	return 'Options:\n' + '\n'.join(opts)

def modules():
	exp = serctl.__path__[0] + '/ctl*.py'
	files = glob.glob(exp)
	names = [ os.path.basename(i)[:-3][3:] for i in files ]
	names.sort()
	n = len(reduce(_gt, names)) + 1
	mods = [ '\tser_' + i.ljust(n) + '[options...] [--] [[command] params...]' \
	         for i in names ]
	return 'Usage:\n' + '\n'.join(mods)


def help(*tmp):
	print """\
%s

%s

For module's commands and params description use 'ser_<module_name> -h'.
""" % (modules(), options())
