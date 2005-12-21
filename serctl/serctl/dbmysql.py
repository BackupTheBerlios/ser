#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: dbmysql.py,v 1.1 2005/12/21 18:18:30 janakj Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# Created:     2005/11/14
# Last update: 2005/11/28

from dbsql import DBsql
import MySQLdb

class DBmysql(DBsql):
	dbmodule = MySQLdb
