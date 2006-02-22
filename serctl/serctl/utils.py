#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: utils.py,v 1.6 2006/02/22 22:53:21 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.error   import Error, EINVAL, EMISMATCH, EALL
from serctl.options import OPT_DATABASE, OPT_LIMIT, OPT_REC_SEP, OPT_LINE_SEP, \
                           OPT_TABLE, OPT_COL_SEP, OPT_COLUMNS, OPT_FORCE, OPT_NUMERIC
from serctl.config  import config
from time    import strftime, gmtime
import sys

def arg_pairs(args):
	n = len(args)
	if n%2:
		raise Error (EMISMATCH, ' '.join(args))
	adict = {}
	alist = []
	for i in range(0, n, 2):
		k = args[i].lower()
		v = args[i+1]
		adict[k] = v
		alist.append((k, v))
	return adict, alist

def no_all(opts, *args):
	for arg in args:
		if arg is not None:
			return
	if opts.has_key(OPT_FORCE):
		return
	raise Error (EALL, )

def unesc_psep(str):
	li = str.split(r'\\')
	li = [ i.replace(r'\n', '\n') for i in li ]
	li = [ i.replace(r'\t', '\t') for i in li ]
	return '\\'.join(li)

def show_opts(opts):
	limit = opts.get(OPT_LIMIT, 0)
	rsep  = opts.get(OPT_REC_SEP, config.REC_SEP)
	rsep  = unesc_psep(rsep)
	lsep  = opts.get(OPT_LINE_SEP, config.LINE_SEP)
	lsep  = unesc_psep(lsep)
	astab = opts.has_key(OPT_TABLE)
	csep  = opts.get(OPT_COL_SEP, config.COL_SEP)
	cols  = opts.get(OPT_COLUMNS, '')
	cols  = [ i.strip(config.WHITESP) for i in cols.split(csep) ]
	cols  = filter(None, cols)
	num   = opts.has_key(OPT_NUMERIC)
	return (cols, num, limit, rsep, lsep, astab)

def timestamp():
	return strftime('%Y-%m-%d %H:%M:%S', gmtime())

def idx_dict(lst):
	idx = {}
	for i in range(len(lst)):
		idx[lst[i]] = i
	return idx

def tabprint(data, desc, rsep=config.REC_SEP, lsep=config.LINE_SEP, tab=False):
	if not tab:
		for row in data:
			line = rsep.join(row)
			sys.stdout.write(line + lsep)
		return

	rl = len(rsep)
	names = [ d[0] for d in desc ]
	if data:
		n = len(data[0])
		width = [0]*n
		for row in data:
			for i in range(n):
				l = len(row[i])
				if width[i] < l:
					width[i] = l
	else:
		n = len(names)
		width = [0]*n
	for i in range(len(names)):
		l = len(names[i])
		if width[i] < l:
			width[i] = l

	rule = '+'
	for i in range(n):
		rule += '-' * (width[i]+2*rl) + '+'
	sys.stdout.write(rule + lsep)

	line = '|'
	for i in range(n):
		line += rsep + names[i].ljust(width[i]) + rsep + '|'
	sys.stdout.write(line + lsep)
	sys.stdout.write(rule + lsep)

	for row in data:
		line = '|'
		for i in range(n):
			line += rsep + row[i].ljust(width[i]) + rsep + '|'
		sys.stdout.write(line + lsep)
	sys.stdout.write(rule + lsep)

def var2tab(data, dsc=None):
	if type(data) == dict:
		ret  = [ (str(k), str(v)) for k, v in data.items() ]
		desc = [ ('key', '?', ''), ('value', '?', '') ]
	elif type(data) == tuple or type(data) == list:
		ret  = [ (str(i), ) for i in data ]
		desc = [ ('value', '?', ''), ]
	else:
		ret  = [ (str(data), ) ] 
		desc = [ ('value', '?', ''), ]
	if dsc is not None:
		if type(dsc) == str:
			dsc = [dsc,]
		desc = [ (i, '?', '') for i in dsc ]
	return ret, desc

def dict2tab(data, keys=None, dsc=None):
	if keys is None:
		keys = data.keys()
		keys.sort()
	if dsc is None:
		dsc = [ str(i).replace('_', ' ') for i in keys ]
	ret = []
	for k in keys:
		ret.append(str(data[k]))
	ret = [ret]
	desc = [ (i, '?', '') for i in dsc ]
	return ret, desc

