#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: error.py,v 1.10 2006/03/24 17:08:05 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from errno  import *
from serctl.config import NAME
import os, sys

ERRORS = (\
#	('ECODE', 	'Description')

	('ERESERVED',	'Reserved error code for testing'),
	('ENODB',	'Database URI not specified'),
	('EMISMATCH',	'Argument mismatch'),
	('ENOARG',	'Argument required'),
	('ENOUSER',	'User not exist'),
	('ENODOMAIN',	'Domain not exist'),
	('EIFLAG',	'Invalid flag'),
	('EDUPL',	'Duplicated'),
	('ENOCOL',	'Invalid column name'),
	('ERMCANON',	'Canonical item can not be deleted'),
	('EDOMAIN',	'Domain identifier in use'),
	('ENOREC',	'No record found'),
	('ENOUID',	'User identifier not exist'),
	('ENODID',	'Domain identifier not exist'),
	('EMULTICANON',	'Multiple specifications for canonical flag'),
	('EUSER',	'User identifier in use'),
	('EALL',	'For operation with ALL items use force flag'),
	('EUMAP',	'Auth_username & realm mapped to many uids'),
	('ENOSER',	'SER URI not specified'),
	('ENOCANON',	'No canonical record found'),
	('ENOALIAS',	'No username or alias found'),
	('EICONF',	'Invalid config file parameter'),
	('EINAME',	'Invalid program name invocation'),
	('ENOHELP',	'Sorry, help not yet done'),
	('EEXTRA',	'Unknown extra argument(s)'),
	('ECOLSPEC',	'Invalid column specification'),
	('ERPC',	'RPC error'),
)

i = 384
errorstr               = {}
for erc, desc in ERRORS:
	exec erc + ' = ' + str(i)
	errorcode[i] = erc
	errorstr[i]  = desc
	i += 1
del i, erc, desc

def strerror(err):
	if err < 256:
		return os.strerror(err)
	if not errorstr.has_key(err):
		return 'Unknown error ' + str(err)
	return errorstr[err]

def error(err, text=''):
	text = str(text)
	if text:
		return text + ': ' + strerror(err)
	return strerror(err)

class Error(Exception):
	def __init__(self, err, text=''):
		self.err  = err
		self.text = text

	def __str__(self):
		return error(self.err, self.text)

def excepthook(type, value, traceback):
	type  = str(type).split('.')[-1]
	value = str(value)
	if type == 'Error':
		str_text = NAME + ': ' + value + '\n'
	else:
		if value:
			str_text = NAME + ': ' + value + ': ' + type + '\n'
		else:
			str_text = NAME + ': ' + type + '\n'
	sys.stderr.write(str_text)

def set_excepthook(debug=False):
	if debug:
		sys.excepthook = orig_excepthook
	else:
		sys.excepthook = excepthook


def warning(msg):
	sys.stderr.write('WARNING: ' + str(msg) + '\n')

orig_excepthook = sys.excepthook
set_excepthook()
