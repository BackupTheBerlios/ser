#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# Pure python binrpc implementation.
# 

import socket, random
random.seed()

### magic & version
BINRPC_MAGIC	= 0x0A
BINRPC_VERS	= 1
BINRPC_MAGVER	= BINRPC_MAGIC << 4 | BINRPC_VERS

### message types
BINRPC_REQ	= 0
BINRPC_REPL	= 1
BINRPC_FAULT	= 3

### values types
BINRPC_T_INT	= 0
BINRPC_T_STR	= 1
BINRPC_T_DOUBLE	= 2
BINRPC_T_STRUCT	= 3
BINRPC_T_ARRAY	= 4
BINRPC_T_AVP	= 5
BINRPC_T_BYTES	= 6

### sizes & offsets
BINRPC_MIN_HDR_SIZE	= 4
MAX_REPLY_SIZE		= 4096


class BinrpcError(Exception):
	def __init__(self, msg=''):
		self.msg = str(msg)

	def __str__(self):
		return self.msg

def i2be(i):
	s = ''
	if i == 0:
		return chr(0)
	while (i):
		s += chr(i & 0xff)
		i = i >> 8
	return s

def be2i(s):
	i = 0
	while (len(s)):
		i = (i << 8) | ord(s[0])
		s = s[1:]
	return i

def mkhdr(msgtype, bodylen, cookie):
	hdr = chr(BINRPC_MAGVER)
	if bodylen > 65535 or bodylen < 0:
		raise BinrpcError, 'Message is too long'
	bl = i2be(bodylen)
	if cookie > 65535 or cookie < 0:
		raise BinrpcError, 'Invalid cookie'
	co = i2be(cookie)
	hdr += chr(((msgtype << 4) | ((len(bl) - 1) << 2) | (len(co) - 1)) & 0xff)
	hdr += bl
	hdr += co
	return hdr

def _mkrec(type, s):
	l = len(s)
	if l < 8:
		return chr((l << 4) | type) + s
	length = i2be(l)
	ll = len(length)
	if ll > 7:
		raise BinrpcError, 'Message is too long'
	return chr((ll << 4) | 0x80 | type) + length + s

def mkrec(a):
	if a == 0:
		return chr(0)
	if type(a) == int:
		i = i2be(a)
		return _mkrec(BINRPC_T_INT, i)
	if type(a) == str:
		a += '\0'
		return _mkrec(BINRPC_T_STR, a)
	raise BinrpcError, 'Unsupported variable type'

def mkreq(cookie, method, args=[]):
	msg = mkrec(method)
	for a in args:
		msg += mkrec(a)
	hdr = mkhdr(BINRPC_REQ, len(msg), cookie)
	return hdr + msg

def parse_hdr(s):
	if len(s) < BINRPC_MIN_HDR_SIZE:
		raise BinrpcError, 'Reply is too short'
	if ord(s[0]) != BINRPC_MAGVER:
		raise BinrpcError, 'Invalid magic or version'
	i = ord(s[1])
	cl = (i & 0x03) + 1
	ll = ((i >> 2) & 0x03) + 1
	flags = (i >> 4) & 0x0f
	l = be2i(s[2:2+ll])
	c = be2i(s[2+ll:2+ll+cl])
	return flags, c, l, s[2+ll+cl:]

def parse_rec(s):
	if not s:
		raise BinrpcError, 'Record is empty'
	x = ord(s[0])
	if x == 0:
		return 0, s[1:]
	rtype = x & 0x0f
	size = (x >> 4) & 0x07
	if x & 0x80:
		if size == 0:
			if rtype == BINRPC_T_STRUCT or rtype == BINRPC_T_ARRAY:
				return None, s[1:]	# end-mark
			raise BinrpcError, 'Incorrect record size'
		l = be2i(s[1:size+1])
		s = s[size+1:]
		size = l
	else:
		s = s[1:]
	if rtype == BINRPC_T_INT:
		return be2i(s[:size]), s[size:]
	if rtype == BINRPC_T_STR:
		return s[:size-1], s[size:]
	if rtype == BINRPC_T_DOUBLE:
		i =  be2i(s[:size])
		return i/1000.0, s[size:]
	if rtype == BINRPC_T_AVP:
		name = s[:size-1]
		value, s = parse_rec(s[size:])
		return (name, value), s
	if rtype == BINRPC_T_STRUCT:
		d = {}
		while(1):
			e, s = parse_rec(s)
			if e is None: break
			if type(e) != tuple:
				raise BinrpcError, 'Incorrect structure member'
			d[e[0]] = e[1]
		return d, s
	if rtype == BINRPC_T_ARRAY:
		l = []
		while(1):
			e, s = parse_rec(s)
			if e is None: break
			l.append(e)
		return l, s
	if rtype == BINRPC_T_BYTES:
		return s[:size], s[size:]
	raise BinrpcError, 'Unknown reply type'

def parse_msg(s):
	data = []
	while s:
		x, s = parse_rec(s)
		data.append(x)
	return data

def rpc(sck, cmd, args=[]):
	cookie = random.randint(0,255)
	req = mkreq(cookie, cmd, args)
	sck.sendall(req)
	reply = sck.recv(MAX_REPLY_SIZE)
	flags, retco, length, msg = parse_hdr(reply)
	if not flags in (BINRPC_REPL, BINRPC_FAULT):
		raise BinrpcError, 'Incorrect reply flags'
	if retco != cookie:
		raise BinrpcError, 'Invalid cookie in reply'
	if length != len(msg):
		raise BinrpcError, 'Incorrect message length'
	data = parse_msg(msg)
	if flags == BINRPC_FAULT:
		raise BinrpcError, str(data[0]) + ' ' + data[1]
	if len(data) == 1:
		return data[0]
	return data

def mksockus(filename="/tmp/ser_ctl"):
	sck = socket.socket(socket.AF_UNIX)
	sck.connect(filename)
	return sck

def mksockud(filename="/tmp/ser_ctl"):
	sck = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
	sck.connect(filename)
	return sck

if __name__ == "__main__":
	sck = mksockus()
	print rpc(sck, 'core.ps')
