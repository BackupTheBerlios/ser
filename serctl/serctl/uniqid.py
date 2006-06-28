#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: uniqid.py,v 1.2 2006/06/28 13:33:49 hallik Exp $
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
ID_UUID = 'uuid1'
ID_UUID1 = 'uuid1'
ID_UUID3 = 'uuid3'
ID_UUID4 = 'uuid4'
ID_UUID5 = 'uuid5'

IDS = {\
	'0'        : ID_URI,
	'uri'      : ID_URI,
	'ur'       : ID_URI,

	'1'        : ID_UUID1,
	'uuid'     : ID_UUID1,
	'uui'      : ID_UUID1,
	'uu'       : ID_UUID1,
	'u'        : ID_UUID1,
	'uid'      : ID_UUID1,
	'id'       : ID_UUID1,
	'ui'       : ID_UUID1,

	'1'        : ID_UUID1,
	'uuid1'     : ID_UUID1,
	'uui1'      : ID_UUID1,
	'uu1'       : ID_UUID1,
	'u1'        : ID_UUID1,
	'uid1'      : ID_UUID1,
	'id1'       : ID_UUID1,
	'ui1'       : ID_UUID1,

	'2'        : ID_INT,
	'integer'  : ID_INT,
	'intege'   : ID_INT,
	'integ'    : ID_INT,
	'inte'     : ID_INT,
	'int'      : ID_INT,
	'in'       : ID_INT,
	'i'        : ID_INT,

	'3'        : ID_UUID3,
	'uuid3'    : ID_UUID3,
	'uui3'     : ID_UUID3,
	'uu3'      : ID_UUID3,
	'u3'       : ID_UUID3,
	'uid3'     : ID_UUID3,
	'id3'      : ID_UUID3,
	'ui3'      : ID_UUID3,

	'4'        : ID_UUID4,
	'uuid4'    : ID_UUID4,
	'uui4'     : ID_UUID4,
	'uu4'      : ID_UUID4,
	'u4'       : ID_UUID4,
	'uid4'     : ID_UUID4,
	'id4'      : ID_UUID4,
	'ui4'      : ID_UUID4,

	'5'        : ID_UUID5,
	'uuid5'    : ID_UUID5,
	'uui5'     : ID_UUID5,
	'uu5'      : ID_UUID5,
	'u5'       : ID_UUID5,
	'uid5'     : ID_UUID5,
	'id5'      : ID_UUID5,
	'ui5'      : ID_UUID5,
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

def uuid(ver=1,  ns='http://iptel.org/'):
	if ver in (3, 5):
		ns = ' ns:URL ' + ns
	else:
		ns = ''
	ver = ' -v' + str(ver)
	st, out = commands.getstatusoutput('uuid' + ver + ns)
	if st != 0:
		raise Error (EUUIDCMD, out)
	return out

def id(s, idtype=ID_URI):
	if idtype == ID_URI:
		return s
	if idtype == ID_INT:
		raise Error (ENOSYS, 'Unique incremetal integer ID')
	if idtype == ID_UUID1:
		return uuid(1)
	if idtype == ID_UUID3:
		return uuid(3)
	if idtype == ID_UUID4:
		return uuid(4)
	if idtype == ID_UUID5:
		return uuid(5)
	raise Error (EIDTYPE, str(idtype))
