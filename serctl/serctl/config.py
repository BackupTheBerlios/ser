#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: config.py,v 1.16 2008/05/20 09:48:33 kozlik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

import sys, os.path

### --- Global variable definitions --- ###

#
# Default config file path.
#
CONFIG = '/etc/serctl/serctl.conf'

#
# Database URI, this should be the SER database.
#
DB_URI = 'mysql://ser:heslo@localhost/ser'

#
# Ser URI, this should be the SER URI for xmlrpc requests.
#
SER_URI = 'unix:/tmp/ser_ctl'


### --- End of global variable definitions. --- ###
# Don't edit lines below until you know what you are doing.

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
