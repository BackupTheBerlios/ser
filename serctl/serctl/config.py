#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: config.py,v 1.14 2006/03/08 23:27:52 hallik Exp $
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
# Database URI, this should be the SER database
#
DB_URI = 'mysql://ser:heslo@localhost/ser'

#
# Ser URI, this should be the SER URI for xmlrpc requests.
#
SER_URI = 'http://localhost:5060/'


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
