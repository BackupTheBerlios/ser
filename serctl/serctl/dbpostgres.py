#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: dbpostgres.py,v 1.1 2006/01/12 12:13:15 janakj Exp $
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
from uri import USER,PASS,HOST,PORT,DB
import psycopg

class DBpostgres(DBsql):
	dbmodule = psycopg

	def __init__(self, puri):
		params  = []
		if puri[USER] is not None:
			params.append("user='%s'" % puri[USER])
		if puri[PASS] is not None:
			params.append("password='%s'" % puri[PASS])
		if puri[HOST] is not None:
			params.append("host='%s'" % puri[HOST])
		if puri[PORT] is not None:
			params.append("port='%s'" % puri[PORT])
		if puri[DB] is not None:
			params.append("database='%s'" % puri[DB])
		params = ', '.join(params)
		connect = 'self.dbmodule.connect(%s)' % params
		self.db = eval(connect)
