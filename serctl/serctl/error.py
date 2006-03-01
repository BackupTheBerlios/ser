#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: error.py,v 1.7 2006/03/01 10:51:22 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from errno  import *
from serctl.config import config
import os, sys

errorstr               = {}

ERESERVED              = 384
errorcode[ERESERVED]   = 'ERESERVED'
errorstr[ERESERVED]    = 'Reserved error code for testing'

ENODB                  = 385
errorcode[ENODB]       = 'ENODB'
errorstr[ENODB]        = 'Database URI not specified'

EMISMATCH              = 386
errorcode[EMISMATCH]   = 'EMISMATCH'
errorstr[EMISMATCH]    = 'Argument mismatch'

ENOARG                 = 387
errorcode[ENOARG]      = 'ENOARG'
errorstr[ENOARG]       = 'Argument required'

ENOUSER                = 388
errorcode[ENOUSER]     = 'ENOUSER'
errorstr[ENOUSER]      = 'User not exist'

ENODOMAIN              = 389
errorcode[ENODOMAIN]   = 'ENODOMAIN'
errorstr[ENODOMAIN]    = 'Domain not exist'

EIFLAG                 = 390
errorcode[EIFLAG]      = 'EIFLAG'
errorstr[EIFLAG]       = 'Invalid flag'

EDUPL                  = 391
errorcode[EDUPL]       = 'EDUPL'
errorstr[EDUPL]        = 'Duplicated'

ENOCOL                 = 392
errorcode[ENOCOL]      = 'ENOCOL'
errorstr[ENOCOL]       = 'Invalid column name'

ERMCANON               = 393
errorcode[ERMCANON]    = 'ERMCANON'
errorstr[ERMCANON]     = 'Canonical item can not be deleted'

EDOMAIN                = 394
errorcode[EDOMAIN]     = 'EDOMAIN'
errorstr[EDOMAIN]      = 'Domain identifier in use'

ENOREC                 = 395
errorcode[ENOREC]      = 'ENOREC'
errorstr[ENOREC]       = 'No record found'

ENOUID                 = 396
errorcode[ENOUID]      = 'ENOUID'
errorstr[ENOUID]       = 'User identifier not exist'

ENODID                 = 397
errorcode[ENODID]      = 'ENODID'
errorstr[ENODID]       = 'Domain identifier not exist'

EMULTICANON            = 398
errorcode[EMULTICANON] = 'EMULTICANON'
errorstr[EMULTICANON]  = 'Multiple specifications for canonical flag'

EUSER                  = 399
errorcode[EUSER]       = 'EUSER'
errorstr[EUSER]        = 'User identifier in use'

EALL                   = 400
errorcode[EALL]        = 'EALL'
errorstr[EALL]         = 'For operation with ALL items use force flag'

EUMAP                  = 401
errorcode[EUMAP]       = 'EUMAP'
errorstr[EUMAP]        = 'Auth_username & realm mapped to many uids'

ENOSER                 = 402
errorcode[ENOSER]      = 'ENOSER'
errorstr[ENOSER]       = 'SER URI not specified'

ENOCANON               = 403
errorcode[ENOCANON]    = 'ENOCANON'
errorstr[ENOCANON]     = 'No canonical record found'

ENOALIAS               = 403
errorcode[ENOALIAS]    = 'ENOALIAS'
errorstr[ENOALIAS]     = 'No username or alias found'


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
		str_text = config.NAME + ': ' + value + '\n'
	else:
		if value:
			str_text = config.NAME + ': ' + value + ': ' + type + '\n'
		else:
			str_text = config.NAME + ': ' + type + '\n'
	sys.stderr.write(str_text)

def set_excepthook(debug=False):
	if debug:
		sys.excepthook = old_excepthook
	else:
		sys.excepthook = excepthook


def warning(msg):
	sys.stderr.write('WARNING: ' + str(msg) + '\n')

old_excepthook = sys.excepthook
set_excepthook(config.DEBUG)

