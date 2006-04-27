#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: utils.py,v 1.12 2006/04/27 22:32:20 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.error   import Error, EINVAL, EMISMATCH, EALL, ENOCOL, EINT, \
                           EIDTYPE
from serctl.options import OPT
from flag           import CND_NO_DELETED
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

def arg_attrs(args):
	aa = []
	for a in args:
		aa += a.split('=', 1)
	return arg_pairs(aa)

def no_all(opts, *args):
	for arg in args:
		if arg is not None:
			return
	if opts['FORCE']:
		return
	raise Error (EALL, )

def unesc_psep(str):
	li = str.split(r'\\')
	li = [ i.replace(r'\n', '\n') for i in li ]
	li = [ i.replace(r'\t', '\t') for i in li ]
	return '\\'.join(li)

def show_opts(opts):
	limit = opts['LIMIT']
	try:
		limit = int(limit)
	except:
		raise Error (EINT, '--limit')
	rsep  = unesc_psep(opts['REC_SEP'])
	lsep  = unesc_psep(opts['LINE_SEP'])
	astab = opts['AS_TABLE']
	cols  = opts['COLUMNS']
	cols  = [ i.strip(' \t') for i in cols.split(',') ]
	cols  = filter(None, cols)
	if opts['RAW']:
		fformat = 'raw'
	else:
		fformat = opts['FFORMAT']
	return (cols, fformat, limit, rsep, lsep, astab)

def timestamp():
	return strftime('%Y-%m-%d %H:%M:%S', gmtime())

def idx_dict(lst):
	idx = {}
	for i in range(len(lst)):
		idx[lst[i]] = i
	return idx

def col_idx(col_idx, cols):
	idx = []
	for col in cols:
		try:
			i = col_idx[col]
		except KeyError:
			raise Error (ENOCOL, col)
		idx.append(i)
	return tuple(idx)


def cond(*args, **kwargs):
	cnd =  ['and', 1]
	for arg in args:
		cnd.append(arg)
	err = []
	for k, v in kwargs.items():
		if v is None: continue
		cnd.append(('=', k, v))
		err.append(k + '=' + str(v))
	err = ' '.join(err)
	return (cnd, err)

def errstr(**kwargs):
	err = []
	for k, v in kwargs.items():
		if not v: continue
		err.append(k + '=' + str(v))
	err = ' '.join(err)
	return err

def full_cond(columns, values):
	cv = zip(columns, values)
	cnd = ['and', CND_NO_DELETED] + [ ('=', c, v) for c, v in cv ]
	return tuple(cnd)

def tabprint(data, desc, rsep=OPT['REC_SEP'][3], lsep=OPT['LINE_SEP'][3], tab=False):
	if not tab:
		for row in data:
			line = rsep.join(row)
			sys.stdout.write(line + lsep)
		return

	rrsep = list(rsep)
	rrsep.reverse()
	rrsep = ''.join(rrsep)
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
		line += rsep + names[i].ljust(width[i]) + rrsep + '|'
	sys.stdout.write(line + lsep)
	sys.stdout.write(rule + lsep)

	for row in data:
		line = '|'
		for i in range(n):
			line += rsep + row[i].ljust(width[i]) + rrsep + '|'
		sys.stdout.write(line + lsep)
	sys.stdout.write(rule + lsep)

def var2tab(data, dsc=None):
	if type(data) == dict:
		ret  = [ (str(k), str(v)) for k, v in data.items() ]
		desc = [ ('key', None, ''), ('value', None, '') ]
	elif type(data) == tuple or type(data) == list:
		ret  = [ (str(i), ) for i in data ]
		desc = [ ('value', None, ''), ]
	else:
		ret  = [ (str(data), ) ] 
		desc = [ ('value', None, ''), ]
	if dsc is not None:
		if type(dsc) == str:
			dsc = [dsc,]
		desc = [ (i, None, '') for i in dsc ]
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
	desc = [ (i, None, '') for i in dsc ]
	return ret, desc

def uniq(items):
	d = {}
	for i in items:
		d[i] = None
	return d.keys()

def id(s, idtype='orig'):
	if idtype == 'orig':
		return s
	raise Error (EIDTYPE, idtype)
