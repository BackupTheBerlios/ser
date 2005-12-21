#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: opsql.py,v 1.1 2005/12/21 18:18:30 janakj Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# Created:     2005/12/11
# Last update: 2005/12/11


def quote(s):
	return "'" + s.replace("'", r"\'") + "'"

def where(conds):
	op = conds[0]
	args = []
	for arg in conds[1:]:
		if type(arg) in (tuple, list):
			arg = '(' + where(arg) + ')'
		args.append(arg)
	return OP[op](args)

### Operators:

def quote_(arg):
	return quote(arg[0])

def eq_cq(arg):
	if arg[1] is None:
		return str(arg[0]) + ' IS NULL'
	return str(arg[0]) + '=' + quote(str(arg[1]))

def neq_cq(arg):
	if arg[1] is None:
		return str(arg[0]) + ' IS NOT NULL'
	return str(arg[0]) + '!=' + quote(str(arg[1]))

def or_(arg):
	arg = [str(i) for i in arg]
	return ' OR '.join(arg)

def and_(arg):
	arg = [str(i) for i in arg]
	return ' AND '.join(arg)

def not_(arg):
	return 'NOT ' + str(arg[0])

def bit_and_(arg):
	arg = [str(i) for i in arg]
	return ' & '.join(arg)

def bit_or_(arg):
	arg = [str(i) for i in arg]
	return ' & '.join(arg)

OP = {
	'"'   : quote_,
	'='   : eq_cq,
	'!='  : neq_cq,
	'not' : not_,
	'and' : and_,
	'or'  : or_,
	'&'   : bit_and_,
	'|'   : bit_or_,
}
