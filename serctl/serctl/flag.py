#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: flag.py,v 1.8 2006/04/27 22:32:20 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

from serctl.error import Error, EIFLAG, EFFORM

# Flag values from sip_router/db/db.h:
LOAD_SER       = 1L << 0  # The row should be loaded by SER 
DISABLED       = 1L << 1  # The row is disabled
CANON          = 1L << 2  # Canonical entry (domain or uri)
IS_TO          = 1L << 3  # The URI can be used in To
IS_FROM        = 1L << 4  # The URI can be used in From
FOR_SERWEB     = 1L << 5  # Credentials instance can be used by serweb
PENDING        = 1L << 6
DELETED        = 1L << 7
CALLER_DELETED = 1L << 8  # Accounting table
CALLEE_DELETED = 1L << 9  # Accounting table

RESERVED       = 1L << 31 # Bit reserved for internal use.
                          # Distinguish between set/unset flag

AND_MASK       = 0xFFFFFFFFL
OR_MASK        = 0L

FLAGS = {
	'c' : CANON,
	'd' : DISABLED,
	'e' : CALLEE_DELETED,
	'f' : IS_FROM,
	'p' : PENDING,
	'r' : CALLER_DELETED,
	's' : LOAD_SER,
	't' : IS_TO,
	'w' : FOR_SERWEB,
	'x' : DELETED,
}

CND_NO_DELETED   = ('not', ('!=', ('&', 'flags', DELETED), 0))
CND_DELETED      =         ('!=', ('&', 'flags', DELETED), 0)
CND_CANONICAL    =         ('!=', ('&', 'flags', CANON), 0)
CND_NO_CANONICAL =         ('=',  ('&', 'flags', CANON), 0)

FFORMAT = { \
	'raw'         : 'raw',
	'ra'          : 'raw',
	'r'           : 'raw',

	'symbolic'    : 'sym',
	'symboli'     : 'sym',
	'symbol'      : 'sym',
	'symbo'       : 'sym',
	'symb'        : 'sym',
	'sym'         : 'sym',
	'sy'          : 'sym',
	's'           : 'sym',

	'hexadecimal' : 'hex',
	'hexadecima'  : 'hex',
	'hexadecim'   : 'hex',
	'hexadeci'    : 'hex',
	'hexadec'     : 'hex',
	'hexade'      : 'hex',
	'hexad'       : 'hex',
	'hexa'        : 'hex',
	'hex'         : 'hex',
	'he'          : 'hex',
	'h'           : 'hex',

	'octal'       : 'oct',
	'oct'         : 'oct',
	'oc'          : 'oct',
	'o'           : 'oct',

	'decimal'     : 'dec',
	'decima'      : 'dec',
	'decim'       : 'dec',
	'deci'        : 'dec',
	'dec'         : 'dec',
	'de'          : 'dec',
	'd'           : 'dec',
}


def _syms(flags):
	d = {}
	for k, v in flags.items():
		d[v] = k
	s = []
	keys = d.keys()
	keys.sort()
	for k in keys:
		s.append(d[k])
	s.reverse()
	return s

SYMS = _syms(FLAGS)

def flag_syms(flags):
	flags = long(flags)
	s = ''
	for f in SYMS:
		if flags & FLAGS[f]:
			s += f
		else:
			s += '-'
	return s

def flag_hex(flags):
	flags = long(flags)
	flags = hex(flags)
	flags = str(flags)
	flags = flags.rstrip('L')
	return flags

def flag_oct(flags):
	flags = long(flags)
	flags = oct(flags)
	flags = str(flags)
	flags = flags.rstrip('L')
	return flags

def flag_dec(flags):
	flags = long(flags)
	flags = str(flags)
	flags = flags.lstrip('0')
	flags = flags.rstrip('L')
	return flags

def parse_flags(flags):
	if flags is None:
		return (AND_MASK, OR_MASK)
	try:
		i = long(flags)
		return (0L, i)
	except:
		pass
	set = True
	abs = False
	and_mask = AND_MASK
	or_mask  = OR_MASK
	for f in flags:
		if f == '-':
			set = False
			continue
		if f == '+':
			set = True
			continue
		if f == '=':
			abs = True
		try:
			i = FLAGS[f]
		except KeyError:
			raise Error (EIFLAG, f)
		if set:
			or_mask |= i
		else:
			and_mask &= ~i
	if abs:
		mask = or_mask | ~and_mask
		return (mask, mask)
	return (and_mask, or_mask)


def cv_flags(format, flags):
	try:
		format = FFORMAT[format]
	except KeyError:
		raise Error (EFFORM, format)
	if format == 'raw':
		return flags
	if format == 'sym':
		return flag_syms(flags)
	if format == 'hex':
		return flag_hex(flags)
	if format == 'oct':
		return flag_oct(flags)
	return flag_dec(flags)

def new_flags(old_flags, mask):
	return str(long(old_flags) & mask[0] | mask[1])

def is_canonical(flags):
	return (long(flags) & CANON) > 0

def set_deleted(flags):
	return str(long(flags) | DELETED)

def set_canonical(flags):
	return str(long(flags) | CANON)

def clear_canonical(flags):
	return str(long(flags) & ~CANON)
