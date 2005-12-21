#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: dbbase.py,v 1.1 2005/12/21 18:18:30 janakj Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# Created:     2005/11/14
# Last update: 2005/12/06

from error import Error, ENOSYS

DSC_NAME    = 0
DSC_TYPE    = 1
DSC_DEFAULT = 2

class DBbase:

	def select(self, tab, cols, conds, limit=0):
		raise Error (ENOSYS, 'select')

	def describe(self, tab):
		raise Error (ENOSYS, 'describe')

	def insert(self, tab, ins):
		raise Error (ENOSYS, 'insert')

	def update(self, tab, ins, conds, limit=0):
		raise Error (ENOSYS, 'update')

	def delete(self, tab, conds, limit=0):
		raise Error (ENOSYS, 'delete')
