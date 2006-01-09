#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlraw.py,v 1.2 2006/01/09 13:53:44 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# Created:     2005/12/06
# Last update: 2005/12/15

from dbany   import DBany, DSC_NAME, DSC_DEFAULT
from error   import Error, ENOARG
from options import OPT_DATABASE, CMD_ADD, CMD_RM, CMD_SHOW, OPT_FORCE, \
                    OPT_LIMIT
from utils   import show_opts, arg_pairs, tabprint
import ctlhelp

def main(args, opts):
	cmd = args[2]
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
	return ret

def help(args, opts):
	return """
Usage: serraw raw <add|rm|show> <table_name> [column_name column_value] ...
""" % ctlhelp.options(args, opts)

def show(db, tab, args, opts):

	cols, numeric, limit, rsep, lsep,  astab = show_opts(opts)

	u = Raw(db)
	data, desc = u.show(tab, cols, None, limit)

	tabprint(data, desc, rsep, lsep, astab)

def add(db, tab, args, opts):
	arg = arg_pairs(args)[0]

	u = Raw(db)
	u.add(tab, arg)

def rm(db, tab, args, opts):
	arg = arg_pairs(args)[1]

	limit = opts.get(OPT_LIMIT, 0)

	u = Raw(db)
	u.rm(tab, arg, limit)

class Raw:

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
