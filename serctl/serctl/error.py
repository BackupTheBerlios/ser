#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: error.py,v 1.1 2005/12/21 18:18:30 janakj Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# Created:     2005/11/14
# Last update: 2005/12/02

from errno  import *
import os, sys, config

OS_ERR_PREFIX          = config.NAME + ': '

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
errorstr[EALL]         = 'For operations on entire table use force flag'

EUMAP                  = 401
errorcode[EUMAP]       = 'EUMAP'
errorstr[EUMAP]        = 'Auth_username & realm mapped to many uids'

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
		str_text = OS_ERR_PREFIX + value + '\n'
	else:
		if value:
			str_text = OS_ERR_PREFIX + value + ': ' + type + '\n'
		else:
			str_text = OS_ERR_PREFIX + type + '\n'
	sys.stderr.write(str_text)

def set_excepthook():
	if config.DEBUG:
		sys.excepthook = old_excepthook
	else:
		sys.excepthook = excepthook

old_excepthook = sys.excepthook
set_excepthook()
