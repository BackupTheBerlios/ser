#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: serxmlrpc.py,v 1.2 2006/01/18 11:01:44 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from urllib  import unquote
from urllib2 import parse_http_list, parse_keqv_list
import httplib, xmlrpclib, re, md5

# indexes to parsed uri sequence
SCHEME = 0
USER   = 1
PASS   = 2
HOST   = 3
PORT   = 4

_EXP = re.compile('(^[^:]*)://([^/]*)/?.*$')

def _split_uri(uri):
	exp      = _EXP.match(uri)
	if not exp:
		return ('http', None, '', 'localhost', '5060')
	scheme   = exp.group(1)
	uphp     = exp.group(2)
	if not uphp:
		return (scheme, None, '', 'localhost', '5060')
	uphp = uphp.split('@')
	if len(uphp) < 2:
		user = None
		passwd = ''
		hp = uphp[0].split(':')
		if len(hp) < 2:
			port = '5060'
		else:
			port = hp[1]
		host = hp[0]
	else:
		up = uphp[0].split(':')
		if len(up) < 2:
			passwd = ''
		else:
			passwd = up[1]
		user = up[0]
		hp = uphp[1].split(':')
		if len(hp) < 2:
			port = '5060'
		else:
			port = hp[1]
		host = hp[0]
	return (scheme, user, passwd, host, port)

def _unquote(i):
	if not i:
		return i
	return unquote(i)

def _parse_uri(uri):
	return [ _unquote(i) for i in _split_uri(uri) ]

class UnsupportedAuthScheme(httplib.HTTPException):
	def __init__(self, scheme):
		self.args = (scheme, )
		self.scheme = scheme

class Transport:
	HEADERS = {"Content-type": "application/x-www-form-urlencoded",
	           "User-Agent"  : "serctl/1.0" }

	def __init__(self, uri=None, ssl=None):
		puri = _parse_uri(uri)
		puri[PORT] = int(puri[PORT])
		if puri[USER]:
			self.user = puri[USER]
			self.password = puri[PASS]
		else:
			self.user = 'anonymous'
			self.password = ''
		self.auth = None

		if puri[SCHEME].lower() == 'https':
			if ssl:
				self.conn = httplib.HTTPSConnection( \
				  puri[HOST], puri[PORT], ssl[0], ssl[1])
			else:
				self.conn = httplib.HTTPSConnection( \
				  puri[HOST], puri[PORT])
		else:
			self.conn = httplib.HTTPConnection(puri[HOST], \
			  puri[PORT])

	def _http_request(self, uripath, body, host):
		headers = self.HEADERS.copy()
		headers['Host'] = host
		if self.auth:
			headers['Authorization'] = self.auth
		self.conn.request("POST", uripath, body, headers)

	def _generate_auth_header(self, scheme, uripath, realm, nonce):
		# variable scheme is silently ignored. It is intended for 
		# multiple authentication schemes - only MD5 scheme supported,
		# now. Value of this variable should be equivalent to 
		# lowercased string of HTTP auth scheme name.
		a1 = "%s:%s:%s" % (self.user, realm, self.password)
		a2 = "POST:" + uripath
		h1 = md5.new(a1).hexdigest()
		h2 = md5.new(a2).hexdigest()
		a3 = "%s:%s:%s" % (h1, nonce, h2)
		h3 = md5.new(a3).hexdigest()
		auth = 'Digest username="%s", realm="%s", nonce="%s", ' \
		       'uri="%s", response="%s"' % (self.user, realm, nonce, \
		        uripath, h3)
		return auth

	def _parse_auth_header(self, authhdr):
		l = authhdr.split(' ', 1)
		if len(l) != 2:
			return None, {}
		method = l[0].lower()
		challenge = parse_keqv_list(parse_http_list(l[1]))
		return method, challenge

	def request(self, host, uripath, body, verbose=0):

		self._http_request(uripath, body, host)
		response = self.conn.getresponse()

 		if response.status == 401 or response.status == 407:
			response.read()
			if response.status == 401:
				hdrname = 'WWW-Authenticate'
			else:
				hdrname = 'Proxy-Authenticate'
			authhdr = response.getheader(hdrname)
			scheme, challenge = self._parse_auth_header(authhdr)
			if scheme != 'digest':
				raise UnsupportedAuthScheme (scheme,)

			self.auth = self._generate_auth_header(self, uripath, \
			  challenge['realm'], challenge['nonce'])

			self._http_request(uripath, body, host)
			response = self.conn.getresponse()

		if response.status != 200:
			raise xmlrpclib.ProtocolError(host + uripath, \
			response.status, response.reason, response.msg)

		data = response.read()

		parser, unmarshaller = xmlrpclib.getparser()
		parser.feed(data)
		parser.close()
		return unmarshaller.close()

def ServerProxy(uri, ssl=None, encoding=None, verbose=0, allow_none=0):
	transport = Transport(uri, ssl)
	return xmlrpclib.ServerProxy(uri, transport, encoding, verbose, \
	  allow_none)
