#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: utils.py,v 1.19 2006/10/31 19:40:15 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from getpass        import getpass
from serctl.error   import Error, EINVAL, EMISMATCH, EALL, ENOCOL, EINT, \
                           EPASSWORD
from serctl.options import OPT
from serctl.dbany   import DBany
from serctl.flag    import cv_flags
from flag           import CND_NO_DELETED
from time    import strftime, gmtime
import sys, serctl.options

CND_TRUE  = ['1']
CND_FALSE = ['0']

ID_ORIG = 0
ID_INT  = 1
ID_UUID = 2

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

def uniarg(arg, argdict=serctl.options.CMD):
	try:
		a = argdict[arg]
	except KeyError:
		raise Error (EINVAL, str(arg))
	return a

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
	if cols is None or cols == '*' or cols == '.':
		cols = ''
	cols  = [ i.strip(' \t') for i in cols.split(',') ]
	cols  = filter(None, cols)
	if opts['RAW']:
		fformat = 'raw'
	else:
		fformat = opts['FFORMAT']
	return (cols, fformat, limit, rsep, lsep, astab)

def timestamp():
	return strftime('%Y-%m-%d %H:%M:%S', gmtime())

def get_password(opts, password=None, prompt='Password: ', confirm='Retype: '):
	password = opts.get('PASSWORD', password)
	if password is None:
		password = getpass(prompt)
		retype = getpass(confirm)
		if password != retype:
			raise Error (EPASSWORD, )
	return password

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

class Basectl:
###	This variables should be defined by the child class
#	TABLE = 'table_name'
#	COLUMNS = ('col_name', ... , 'flags')
#	COLIDXS = idx_dict(COLUMNS)
#	FLAGIDX = COLIDXS['flags']

	def __init__(self, dburi, db=None):
		self.dburi  = dburi
		if db is not None:
			self.db = db
		else:
			self.db = DBany(dburi)

	def exist_cnd(self, cnd_err):
		cnd, err = cnd_err
		rows = self.db.select(self.TABLE, None, cnd, limit=1)
		return rows != []

	def show_cnd(self, cnd_err, cols=None, fformat='raw', limit=0):
		if not cols:
			cols = self.COLUMNS
		cidx = col_idx(self.COLIDXS, cols)
		
		cnd, err = cnd_err
		rows = self.db.select(self.TABLE, self.COLUMNS, cnd, limit)

		new_rows = []
		for row in rows:
			row[self.FLAGIDX] = cv_flags(fformat, row[self.FLAGIDX])
			new_row = []
			for i in cidx:
				new_row.append(row[i])
			new_rows.append(new_row)
		desc = self.db.describe(self.TABLE)
		desc = [ desc[i] for i in cols ]
		return new_rows, desc
