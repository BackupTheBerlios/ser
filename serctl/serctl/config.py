#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: config.py,v 1.9 2006/01/30 11:45:49 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# Global constant definitions.

#
# PATH to configuraton file
#
try:
	from localconfig import CONFIG
except:
	CONFIG = '/usr/local/etc/serctl/serctl.conf'

#
# Disable/enable debugging mode
#
DEBUG = False

#
# Default visible name of the tool. 
# Will be replaced by real command name (e.g. ser_domain).
#
NAME  = "serctl"

#
# Verbosity level
#
VERB  = 1

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

###################################
### END OF CONSTANT DEFINITIONS ###
###################################

import sys, os.path

#
# Config file parser.
#

if CONFIG is not None:
	fh = open(CONFIG)
	config_file = fh.read() + '\n'
	fh.close()
	config_file_code = compile(config_file, CONFIG, 'exec')
	del(config_file)
	exec config_file_code
	del(config_file_code)

#
# Determine command name
#

try:
	NAME = os.path.basename(sys.argv[0])
except:
	pass
