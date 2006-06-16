#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: uniqid.py,v 1.1 2006/06/16 12:18:56 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.error   import Error, EIDTYPE, EUUIDCMD, ENOSYS
from serctl.options import OPT
import sys, serctl.options, commands

ID_URI  = 'uri'
ID_INT  = 'int'
ID_UUID = 'uuid'

IDS = {\
	'0'        : ID_URI,
	'uri'      : ID_URI,
	'ur'       : ID_URI,

	'1'        : ID_INT,
	'integer'  : ID_INT,
	'intege'   : ID_INT,
	'integ'    : ID_INT,
	'inte'     : ID_INT,
	'int'      : ID_INT,
	'in'       : ID_INT,
	'i'        : ID_INT,

	'2'        : ID_UUID,
	'uuid'     : ID_UUID,
	'uui'      : ID_UUID,
	'uu'       : ID_UUID,
	'u'        : ID_UUID,
	'uid'      : ID_UUID,
	'id'       : ID_UUID,
	'ui'       : ID_UUID,
}

def idtype(idstr):
	lidstr = idstr.lower()
	id = IDS.get(lidstr)
	if id is None:
		raise Error (EIDTYPE, str(idstr))
	return id

def get_idtype(opts):
	idstr = opts['ID_TYPE']
	id = idtype(idstr)
	return id

def uuid():
	st, out = commands.getstatusoutput('uuid')
	if st != 0:
		raise Error (EUUIDCMD, out)
	return out

def id(s, idtype=ID_URI):
	if idtype == ID_URI:
		return s
	if idtype == ID_INT:
		raise Error (ENOSYS, 'Unique incremetal integer ID')
	if idtype == ID_UUID:
		return uuid()
	raise Error (EIDTYPE, str(idtype))
