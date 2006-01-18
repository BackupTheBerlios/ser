#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: config.py,v 1.7 2006/01/18 14:09:41 hallik Exp $
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
CONFIG = '/usr/local/etc/serctl/serctl.conf'

### CONFIG FILE FOR TESTING ###
import os.path
CONFIG = os.path.join(os.path.dirname(__file__), '../serctl.conf')
del(os.path)
###############################

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
