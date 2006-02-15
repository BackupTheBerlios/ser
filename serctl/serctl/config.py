#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: config.py,v 1.11 2006/02/15 18:51:28 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# Global variable definitions.

import os.path

#
# Default path to config file.
#
CONFIG = "/usr/local/etc/serctl/serctl.conf"

#
# Disable/enable debugging mode
#
DEBUG = False

#
# Database URI, this should be the SER database
#
DB_URI = 'mysql://ser:heslo@localhost/ser'

#
# Name of environment variable used to pass the database
# URI to serctl
#
ENV_DB = 'SERCTL_DB'

#
# Ser URI, this should be the SER URI for xmlrpc requests.
#
SER_URI = 'https://localhost:5060/'

#
# Name of environment variable used to pass the SER URI 
# to serctl
#
ENV_SER = 'SERCTL_SER'

#
# Private key and certificate files for HTTPS
# May be None (both!) if HTTPS transport do not require authentication.
#
SSL_KEY  = '/usr/local/etc/serctl/ser.key'
SSL_CERT = '/usr/local/etc/serctl/ser.cert'

#
# Name of environment variable used to pass the SSL_KEY and the SSL_CERT.
#
ENV_SSL_KEY  = 'SERCTL_SSL_KEY'
ENV_SSL_CERT = 'SERCTL_SSL_CERT'

#
# Miscelaneous global contstants...
#

WHITESP  = ' \t'
REC_SEP  = ' '
LINE_SEP = '\n'
COL_SEP  = ','

#
# Determine visible name of the tool.
#
try:
	NAME = os.path.basename(sys.argv[0])
except:
	NAME = "serctl"

#
# Try load local settings for testing.
#
try:
	from serctl.localconfig import *
except:
	pass

class Config:
	def __setitem__(self, key, value):
		if type(key) != str or key[:1] == '_':
			return
		self.__dict__[key] = value

	def __delitem__(self, key):
		if type(key) != str or key[:1] == '_':
			return
		try:
			del self.__dict__[key]
		except:
			pass

config = Config()

for _k, _v in locals().items():
	if _k in ['Config', 'config']: continue
	config[_k] = _v
del(_k)
del(_v)
