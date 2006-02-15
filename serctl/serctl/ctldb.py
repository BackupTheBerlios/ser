#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctldb.py,v 1.2 2006/02/15 18:51:28 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.dbany   import DBany, DSC_NAME, DSC_DEFAULT
from serctl.error   import Error, ENOARG, EINVAL
from serctl.options import OPT_DATABASE, CMD_ADD, CMD_RM, CMD_SHOW, OPT_FORCE, \
                           OPT_LIMIT, CMD, CMD_HELP
from serctl.utils   import show_opts, arg_pairs, tabprint
import serctl.ctlhelp

def main(args, opts):
	if len(args) < 3:
		print help(args, opts)
		return
	try:
		cmd = CMD[args[2]]
	except KeyError:
		raise Error (EINVAL, args[2])
	if cmd == CMD_HELP:
		print help(args, opts)
		return
	db  = opts[OPT_DATABASE]
	if len(args) < 4:
		raise Error (ENOARG, '<table_name>')
	tab = args[3]
	if   cmd == CMD_ADD:
		ret = add(db, tab, args[4:], opts)
	elif cmd == CMD_RM:
		ret = rm(db, tab, args[4:], opts)
	elif cmd == CMD_SHOW:
		ret = show(db, tab, args[4:], opts)
        else:
                raise Error (EINVAL, cmd)
	return ret

def help(args, opts):
	return """
Usage:
	ser_db [options...] [--] <add|rm|show> <table_name> [column_name column_value] ...

""" + serctl.ctlhelp.options(args, opts)

def show(db, tab, args, opts):

	cols, numeric, limit, rsep, lsep,  astab = show_opts(opts)

	u = Db(db)
	data, desc = u.show(tab, cols, None, limit)

	tabprint(data, desc, rsep, lsep, astab)

def add(db, tab, args, opts):
	arg = arg_pairs(args)[0]

	u = Db(db)
	u.add(tab, arg)

def rm(db, tab, args, opts):
	arg = arg_pairs(args)[1]

	limit = opts.get(OPT_LIMIT, 0)

	u = Db(db)
	u.rm(tab, arg, limit)

class Db:

	def __init__(self, dburi):
		self.db = DBany(dburi)

	def show(self, tab, cols, cond, limit=0):
		desc = self.db.describe(tab)
		if not cols:
			cols = desc.keys()
		head = [ desc[i] for i in cols ]
		return self.db.select(tab, cols, None, limit), head

	def add(self, tab, ins):
		self.db.insert(tab, ins)

	def rm(self, tab, cond, limit=0):
		if not cond:
			return
		cnd = ['and']
		for c in cond:
			cnd.append(('=', c[0], c[1]))
		self.db.delete(tab, cnd, limit)
