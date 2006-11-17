#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: parse_uri.py,v 1.1 2006/11/17 00:17:27 janakj Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# URI: scheme://user:passwd@host:port/database

from urllib import unquote
import re

# indexes to parsed uri sequence
SCHEME = 0
USER   = 1
PASS   = 2
HOST   = 3
PORT   = 4
DB     = 5

EXP = re.compile('(^[^:]*)://([^/]*)/?(.*)$')

def split(uri):
	exp      = EXP.match(uri)
	if not exp:
		return (None, None, None, None, None, None)
	scheme   = exp.group(1)
	uphp     = exp.group(2)
	database = exp.group(3)
	if not uphp:
		return (scheme, None, None, None, None, database)
	uphp = uphp.split('@')
	if len(uphp) < 2:
		user = None
		passwd = None
		hp = uphp[0].split(':')
		if len(hp) < 2:
			port = None
		else:
			port = hp[1]
		host = hp[0]
	else:
		up = uphp[0].split(':')
		if len(up) < 2:
			passwd = None
		else:
			passwd = up[1]
		user = up[0]
		hp = uphp[1].split(':')
		if len(hp) < 2:
			port = None
		else:
			port = hp[1]
		host = hp[0]
	return (scheme, user, passwd, host, port, database)

def _unquote(i):
	if not i:
		return i
	return unquote(i)

def parse(uri):
	return [ _unquote(i) for i in split(uri) ]

def split_sip_uri(uri):
	if uri is None:
		return (None, None)
	n = uri.find(':')
	if n != -1:
		uri = uri.split(':')[1]
	uri  = uri + '@'
	return uri.split('@')[:2]

def adjust_ser_uri(uri):
	uri = uri.strip()
	if uri[:7] != 'http://' and uri[:8] != 'https://':
		uri = 'http://' + uri
	return uri

def _test():
	"""Test strings:

>>> print parse('scheme://user:passwd@host:port/database')
['scheme', 'user', 'passwd', 'host', 'port', 'database']

>>> print parse('scheme://user:@host:port/database')
['scheme', 'user', '', 'host', 'port', 'database']

>>> print parse('scheme://user:@host/database')
['scheme', 'user', '', 'host', None, 'database']

>>> print parse('scheme://user@host/database')
['scheme', 'user', None, 'host', None, 'database']

>>> print parse('://user@host/database')
['', 'user', None, 'host', None, 'database']

>>> print parse('//user@host/database')
[None, None, None, None, None, None]

>>> print parse('scheme://host/database')
['scheme', None, None, 'host', None, 'database']

>>> print parse('scheme://host:port/database')
['scheme', None, None, 'host', 'port', 'database']

>>> print parse('scheme://host')
['scheme', None, None, 'host', None, '']

>>> print parse('scheme://host/')
['scheme', None, None, 'host', None, '']

>>> print parse('scheme:///database')
['scheme', None, None, None, None, 'database']

>>> print parse('scheme://')
['scheme', None, None, None, None, '']

>>> print parse('scheme:///')
['scheme', None, None, None, None, '']

>>> print parse('scheme:////')
['scheme', None, None, None, None, '/']

>>> print parse('://:@:/')
['', '', '', '', '', '']

"""
	import doctest, uri
	return doctest.testmod(uri)

if __name__ == "__main__":
	_test()

