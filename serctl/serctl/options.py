#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: options.py,v 1.7 2006/02/15 12:36:11 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# Command line options.

MOD_ATTR       = 'attr'
MOD_CREDENTIAL = 'credential'
MOD_CTL        = 'ctl'
MOD_DB         = 'db'
MOD_DOMAIN     = 'domain'
MOD_HELP       = 'help'
MOD_INTERCEPT  = 'intercept'
MOD_RPC        = 'rpc'
MOD_URI        = 'uri'
MOD_USER       = 'user'

CMD_ADD        = 'add'
CMD_CANONICAL  = 'canonical'
CMD_DISABLE    = 'disable'
CMD_ENABLE     = 'enable'
CMD_FLUSH      = 'flush'
CMD_HELP       = 'help'
CMD_CHANGE     = 'change'
CMD_PUBLISH    = 'publish'
CMD_PURGE      = 'purge'
CMD_RM         = 'rm'
CMD_SHOW       = 'show'

OPT_COLUMNS    = 'columns'
OPT_COL_SEP    = 'csep'
OPT_CONFIG     = 'config'
OPT_DATABASE   = 'database'
OPT_DEBUG      = 'debug'
OPT_FLAGS      = 'flags'
OPT_FORCE      = 'force'
OPT_HELP       = 'help'
OPT_LIMIT      = 'limit'
OPT_LINE_SEP   = 'lsep'
OPT_NUMERIC    = 'numeric'
OPT_PASSWORD   = 'password'
OPT_REC_SEP    = 'rsep'
OPT_SER_URI    = 'ser-uri'
OPT_SSL_KEY    = 'ssl-key'
OPT_SSL_CERT   = 'ssl-cert'
OPT_TABLE      = 'table'

MOD = {\
	'attr'       : MOD_ATTR,
	'att'        : MOD_ATTR,
	'at'         : MOD_ATTR,
	'a'          : MOD_ATTR,

	'credential' : MOD_CREDENTIAL,
	'credentia'  : MOD_CREDENTIAL,
	'credenti'   : MOD_CREDENTIAL,
	'credent'    : MOD_CREDENTIAL,
	'creden'     : MOD_CREDENTIAL,
	'crede'      : MOD_CREDENTIAL,
	'cred'       : MOD_CREDENTIAL,
	'cre'        : MOD_CREDENTIAL,
	'cr'         : MOD_CREDENTIAL,
	'c'          : MOD_CREDENTIAL,

	'ctl'        : MOD_CTL,
	'ct'         : MOD_CTL,
	'C'          : MOD_CTL,

	'db'         : MOD_DB,

	'domain'     : MOD_DOMAIN,
	'domai'      : MOD_DOMAIN,
	'doma'       : MOD_DOMAIN,
	'dom'        : MOD_DOMAIN,
	'do'         : MOD_DOMAIN,
	'd'          : MOD_DOMAIN,

	'help'       : MOD_HELP,
	'hel'        : MOD_HELP,
	'he'         : MOD_HELP,
	'h'          : MOD_HELP,

	'i'          : MOD_URI,

	'intercept'  : MOD_INTERCEPT,
	'intercep'   : MOD_INTERCEPT,
	'interce'    : MOD_INTERCEPT,
	'interc'     : MOD_INTERCEPT,
	'inter'      : MOD_INTERCEPT,
	'inte'       : MOD_INTERCEPT,
	'int'        : MOD_INTERCEPT,
	'in'         : MOD_INTERCEPT,
	'I'          : MOD_INTERCEPT,

	'rpc'        : MOD_RPC,
	'rp'         : MOD_RPC,
	'R'          : MOD_RPC,

	'uri'        : MOD_URI,
	'ur'         : MOD_URI,

	'user'       : MOD_USER,
	'use'        : MOD_USER,
	'us'         : MOD_USER,
	'u'          : MOD_USER,

	'xmlrpc'     : MOD_RPC,
	'xmlrp'      : MOD_RPC,
	'xmlr'       : MOD_RPC,
	'xml'        : MOD_RPC,
	'x'          : MOD_RPC,
}

CMD = {\
	'add'        : CMD_ADD,
	'ad'         : CMD_ADD,
	'a'          : CMD_ADD,

	'canonical'  : CMD_CANONICAL,
	'canonica'   : CMD_CANONICAL,
	'canonic'    : CMD_CANONICAL,
	'canoni'     : CMD_CANONICAL,
	'canon'      : CMD_CANONICAL,
	'cano'       : CMD_CANONICAL,
	'can'        : CMD_CANONICAL,
	'ca'         : CMD_CANONICAL,
	'c'          : CMD_CANONICAL,

	'disable'    : CMD_DISABLE,
	'disabl'     : CMD_DISABLE,
	'disab'      : CMD_DISABLE,
	'disa'       : CMD_DISABLE,
	'dis'        : CMD_DISABLE,
	'di'         : CMD_DISABLE,
	'd'          : CMD_DISABLE,

	'enable'     : CMD_ENABLE,
	'enabl'      : CMD_ENABLE,
	'enab'       : CMD_ENABLE,
	'ena'        : CMD_ENABLE,
	'en'         : CMD_ENABLE,
	'e'          : CMD_ENABLE,

	'flush'      : CMD_FLUSH,
	'flus'       : CMD_FLUSH,
	'flu'        : CMD_FLUSH,
	'fl'         : CMD_FLUSH,
	'f'          : CMD_FLUSH,

	'help'       : CMD_HELP,
	'hel'        : CMD_HELP,
	'he'         : CMD_HELP,
	'h'          : CMD_HELP,

	'change'     : CMD_CHANGE,
	'chang'      : CMD_CHANGE,
	'chan'       : CMD_CHANGE,
	'cha'        : CMD_CHANGE,
	'ch'         : CMD_CHANGE,
	'C'          : CMD_CHANGE,

	'publish'    : CMD_PUBLISH,
	'publis'     : CMD_PUBLISH,
	'publi'      : CMD_PUBLISH,
	'publ'       : CMD_PUBLISH,
	'pub'        : CMD_PUBLISH,

	'purge'      : CMD_PURGE,
	'purg'       : CMD_PURGE,
	'pur'        : CMD_PURGE,
	'pu'         : CMD_PURGE,
	'p'          : CMD_PURGE,

	'rm'         : CMD_RM,
	'r'          : CMD_RM,

	'show'       : CMD_SHOW,
	'sho'        : CMD_SHOW,
	'sh'         : CMD_SHOW,
	's'          : CMD_SHOW,
}

OPT = {\
#	name          : (short, long,        arg)
	OPT_DATABASE  : ('b', 'database',    True),
	OPT_CONFIG    : ('c', 'config-file', True),
	OPT_COLUMNS   : ('C', 'columns',     True),
	OPT_FLAGS     : ('F', 'flags',       True),
	OPT_FORCE     : ('f', 'force',       False),
	OPT_DEBUG     : ('g', 'debug',       False),
	OPT_HELP      : ('h', 'help',        False),
	OPT_SSL_KEY   : ('k', 'ssl-key',     True),
	OPT_LIMIT     : ('l', 'limit',       True),
	OPT_LINE_SEP  : ('L', 'line-sep',    True),
	OPT_SSL_CERT  : ('m', 'ssl-cert',    True),
	OPT_NUMERIC   : ('n', 'numeric',     False),
	OPT_PASSWORD  : ('p', 'password',    True),
	OPT_REC_SEP   : ('R', 'record-sep',  True),
	OPT_SER_URI   : ('s', 'ser-uri',     True),
	OPT_COL_SEP   : ('S', 'column-sep',  True),
	OPT_TABLE     : ('t', 'table',       False),
}


#--- autogenerated variables ---#

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
