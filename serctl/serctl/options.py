#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: options.py,v 1.21 2006/04/19 14:14:07 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

### Modules, commands and options canonical names:
# Please keep this sorted alfabeticaly by CMD_* name

CMD_ADD        = 'add'
CMD_ALIAS      = 'alias'
CMD_CANONICAL  = 'canonical'
CMD_DISABLE    = 'disable'
CMD_DOMAIN     = 'domain'
CMD_ENABLE     = 'enable'
CMD_FLUSH      = 'flush'
CMD_HELP       = 'help'
CMD_CHANGE     = 'change'
CMD_KILL       = 'kill'
CMD_METHODS    = 'methods'
CMD_PASSWORD   = 'password'
CMD_PS         = 'ps'
CMD_PUBLISH    = 'publish'
CMD_PURGE      = 'purge'
CMD_RELOAD     = 'reload'
CMD_RM         = 'rm'
CMD_SHOW       = 'show'
CMD_STAT       = 'stat'
CMD_UPDATE     = 'update'
CMD_USER       = 'user'
CMD_USRLOC     = 'usrloc'
CMD_UPTIME     = 'uptime'
CMD_VERSION    = 'version'
CMD_LIST_TLS   = 'list_tls'

### Modules and commands command-line representation and abbrevations:
# Please keep this (almost) sorted alfabeticaly by abbrevations (keys).

CMD = {\
	'add'          : CMD_ADD,
	'ad'           : CMD_ADD,
	'a'            : CMD_ADD,

	'alias'        : CMD_ALIAS,
	'alia'         : CMD_ALIAS,
	'ali'          : CMD_ALIAS,
	'al'           : CMD_ALIAS,

	'as'           : CMD_ALIAS,

	'canonical'    : CMD_CANONICAL,
	'canonica'     : CMD_CANONICAL,
	'canonic'      : CMD_CANONICAL,
	'canoni'       : CMD_CANONICAL,
	'canon'        : CMD_CANONICAL,
	'cano'         : CMD_CANONICAL,
	'can'          : CMD_CANONICAL,
	'ca'           : CMD_CANONICAL,
	'c'            : CMD_CANONICAL,

	'disable'      : CMD_DISABLE,
	'disabl'       : CMD_DISABLE,
	'disab'        : CMD_DISABLE,
	'disa'         : CMD_DISABLE,
	'dis'          : CMD_DISABLE,
	'di'           : CMD_DISABLE,
	'd'            : CMD_DISABLE,

	'domain'       : CMD_DOMAIN,
	'domai'        : CMD_DOMAIN,
	'doma'         : CMD_DOMAIN,
	'dom'          : CMD_DOMAIN,
	'do'           : CMD_DOMAIN,

	'enable'       : CMD_ENABLE,
	'enabl'        : CMD_ENABLE,
	'enab'         : CMD_ENABLE,
	'ena'          : CMD_ENABLE,
	'en'           : CMD_ENABLE,
	'e'            : CMD_ENABLE,

	'flush'        : CMD_FLUSH,
	'flus'         : CMD_FLUSH,
	'flu'          : CMD_FLUSH,
	'fl'           : CMD_FLUSH,
	'f'            : CMD_FLUSH,

	'help'         : CMD_HELP,
	'hel'          : CMD_HELP,
	'he'           : CMD_HELP,
	'h'            : CMD_HELP,

	'change'       : CMD_CHANGE,
	'chang'        : CMD_CHANGE,
	'chan'         : CMD_CHANGE,
	'cha'          : CMD_CHANGE,
	'ch'           : CMD_CHANGE,
	'C'            : CMD_CHANGE,

	'kill'         : CMD_KILL,
	'kil'          : CMD_KILL,
	'ki'           : CMD_KILL,
	'k'            : CMD_KILL,

	'list_methods' : CMD_METHODS,
	'list_method'  : CMD_METHODS,
	'list_metho'   : CMD_METHODS,
	'list_meth'    : CMD_METHODS,
	'list_met'     : CMD_METHODS,
	'list_me'      : CMD_METHODS,
	'list_m'       : CMD_METHODS,

	'list_tls'     : CMD_LIST_TLS,
	'list_tl'      : CMD_LIST_TLS,
	'list_t'       : CMD_LIST_TLS,

	'list'         : CMD_SHOW,
	'lis'          : CMD_SHOW,
	'li'           : CMD_SHOW,
	'l'            : CMD_SHOW,

	'lm'           : CMD_METHODS,

	'loc'          : CMD_USRLOC,

	'ls'           : CMD_SHOW,

	'methods'      : CMD_METHODS,
	'method'       : CMD_METHODS,
	'metho'        : CMD_METHODS,
	'meth'         : CMD_METHODS,
	'met'          : CMD_METHODS,
	'me'           : CMD_METHODS,
	'm'            : CMD_METHODS,

	'password'     : CMD_PASSWORD,
	'passwor'      : CMD_PASSWORD,
	'passwo'       : CMD_PASSWORD,
	'passw'        : CMD_PASSWORD,
	'pass'         : CMD_PASSWORD,
	'pas'          : CMD_PASSWORD,
	'pa'           : CMD_PASSWORD,

	'ps'           : CMD_PS,

	'publish'      : CMD_PUBLISH,
	'publis'       : CMD_PUBLISH,
	'publi'        : CMD_PUBLISH,
	'publ'         : CMD_PUBLISH,
	'pub'          : CMD_PUBLISH,

	'purge'        : CMD_PURGE,
	'purg'         : CMD_PURGE,
	'pur'          : CMD_PURGE,
	'pu'           : CMD_PURGE,
	'p'            : CMD_PURGE,

	'pwd'          : CMD_PASSWORD,
	'pw'           : CMD_PASSWORD,

	'reload'       : CMD_RELOAD,
	'reloa'        : CMD_RELOAD,
	'relo'         : CMD_RELOAD,
	'rel'          : CMD_RELOAD,
	're'           : CMD_RELOAD,

	'rm'           : CMD_RM,
	'r'            : CMD_RM,

	'show'         : CMD_SHOW,
	'sho'          : CMD_SHOW,
	'sh'           : CMD_SHOW,
	's'            : CMD_SHOW,

	'status'       : CMD_STAT,
	'statu'        : CMD_STAT,
	'stat'         : CMD_STAT,
	'sta'          : CMD_STAT,
	'st'           : CMD_STAT,

	'tls'          : CMD_LIST_TLS,
	'tl'           : CMD_LIST_TLS,
	't'            : CMD_LIST_TLS,

	'uloc'         : CMD_USRLOC,
	'ul'           : CMD_USRLOC,

	'update'       : CMD_UPDATE,
	'updat'        : CMD_UPDATE,
	'upda'         : CMD_UPDATE,
	'upd'          : CMD_UPDATE,
	'up'           : CMD_UPDATE,

	'uptime'       : CMD_UPTIME,
	'uptim'        : CMD_UPTIME,
	'upti'         : CMD_UPTIME,
	'upt'          : CMD_UPTIME,

	'user'         : CMD_USER,
	'use'          : CMD_USER,
	'us'           : CMD_USER,
	'u'            : CMD_USER,

	'usrloc'       : CMD_USRLOC,
	'usrlo'        : CMD_USRLOC,
	'usrl'         : CMD_USRLOC,
	'usr'          : CMD_USRLOC,

	'version'      : CMD_VERSION,
	'versio'       : CMD_VERSION,
	'versi'        : CMD_VERSION,
	'vers'         : CMD_VERSION,
	'ver'          : CMD_VERSION,
	've'           : CMD_VERSION,
	'v'            : CMD_VERSION,
}


### Option definitions:
# Please keep this sorted alfabeticaly by short form option.

OPT_LIST = (\
#	(short, long,     arg,   default_value)
	('b', 'db-uri',   True,  None),
	('B', 'env-db',   True,  'SERCTL_DB'),
	('c', 'config',   True,  None),
	('C', 'columns',  True,  ''),
	('f', 'force',    False, False),
	('F', 'flags',    True,  None),
	('G', 'dbg-args', False, False),
	('g', 'debug',    False, False),
	('h', 'help',     False, False),
	('j', 'use-fifo', False, False),
	('J', 'fifo',     True,  '/tmp/ser_fifo'),
	('k', 'ssl-key',  True,  None),
	('l', 'limit',    True,  0),
	('L', 'line-sep', True,  '\n'),
	('m', 'ssl-cert', True,  None),
	('n', 'numeric',  False, False),
	('p', 'password', True,       ), # password don't have default value!
	('R', 'rec-sep',  True,  ' '),
	('s', 'ser-uri',  True,  None),
	('S', 'env-ser',  True,  'SERCTL_SER'),
	('t', 'as-table', False, False),
	('U', 'ul-table', True, 'location'),
	('V', 'version',  False, False),
)


### --- autogenerated variables --- ###
# Don't edit lines below until you know what you are doing.

OPT = {}
for v in OPT_LIST:
	OPT[v[1].replace('-', '_').upper()] = v

GETOPT_SHORT = ''.join( [ v[0]       for k, v in OPT.items() if not v[2] ] + \
                        [ v[0] + ':' for k, v in OPT.items() if     v[2] ] )

GETOPT_LONG  =          [ v[1]       for k, v in OPT.items() if not v[2] ] + \
                        [ v[1] + '=' for k, v in OPT.items() if     v[2] ]

OPTS = {}
for k, v in OPT.items():
	OPTS[ '-'  + v[0] ] = k
	OPTS[ '--' + v[1] ] = k
del(k)
del(v)
