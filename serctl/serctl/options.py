#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: options.py,v 1.3 2006/01/06 10:43:45 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# Created:     2005/11/29
# Last update: 2005/12/07
#
# Command line options.

OBJ_ATTR       = 'attr'
OBJ_CREDENTIAL = 'credential'
OBJ_CTL        = 'ctl'
OBJ_DOMAIN     = 'domain'
OBJ_HELP       = 'help'
OBJ_INTERCEPT  = 'intercept'
OBJ_RAW        = 'raw'
OBJ_URI        = 'uri'
OBJ_USER       = 'user'

CMD_ADD        = 'add'
CMD_CANONICAL  = 'canonical'
CMD_DISABLE    = 'disable'
CMD_ENABLE     = 'enable'
CMD_FLUSH      = 'flush'
CMD_HELP       = 'help'
CMD_CHANGE     = 'change'
CMD_PURGE      = 'purge'
CMD_RM         = 'rm'
CMD_SHOW       = 'show'

OPT_COLUMNS    = 'columns'
OPT_COL_SEP    = 'csep'
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
OPT_TABLE      = 'table'
OPT_VERBOSE    = 'verbose'
OPT_QUIET      = 'quiet'

OBJ = {\
	'attr'       : OBJ_ATTR,
	'att'        : OBJ_ATTR,
	'at'         : OBJ_ATTR,
	'a'          : OBJ_ATTR,

	'credential' : OBJ_CREDENTIAL,
	'credentia'  : OBJ_CREDENTIAL,
	'credenti'   : OBJ_CREDENTIAL,
	'credent'    : OBJ_CREDENTIAL,
	'creden'     : OBJ_CREDENTIAL,
	'crede'      : OBJ_CREDENTIAL,
	'cred'       : OBJ_CREDENTIAL,
	'cre'        : OBJ_CREDENTIAL,
	'cr'         : OBJ_CREDENTIAL,
	'c'          : OBJ_CREDENTIAL,

	'ctl'        : OBJ_CTL,
	'ct'         : OBJ_CTL,
	'C'          : OBJ_CTL,

	'domain'     : OBJ_DOMAIN,
	'domai'      : OBJ_DOMAIN,
	'doma'       : OBJ_DOMAIN,
	'dom'        : OBJ_DOMAIN,
	'do'         : OBJ_DOMAIN,
	'd'          : OBJ_DOMAIN,

	'help'       : OBJ_HELP,
	'hel'        : OBJ_HELP,
	'he'         : OBJ_HELP,
	'h'          : OBJ_HELP,

	'i'          : OBJ_URI,

	'intercept'  : OBJ_INTERCEPT,
	'intercep'   : OBJ_INTERCEPT,
	'interce'    : OBJ_INTERCEPT,
	'interc'     : OBJ_INTERCEPT,
	'inter'      : OBJ_INTERCEPT,
	'inte'       : OBJ_INTERCEPT,
	'int'        : OBJ_INTERCEPT,
	'in'         : OBJ_INTERCEPT,
	'I'          : OBJ_INTERCEPT,

	'raw'        : OBJ_RAW,

	'uri'        : OBJ_URI,
	'ur'         : OBJ_URI,

	'user'       : OBJ_USER,
	'use'        : OBJ_USER,
	'us'         : OBJ_USER,
	'u'          : OBJ_USER,
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
#	name          : (short, long,       arg)
	OPT_DATABASE  : ('b', 'database',   True),
	OPT_COLUMNS   : ('c', 'columns',    True),
	OPT_FLAGS     : ('F', 'flags',      True),
	OPT_FORCE     : ('f', 'force',      False),
	OPT_DEBUG     : ('g', 'debug',      False),
	OPT_HELP      : ('h', 'help',       False),
	OPT_LIMIT     : ('l', 'limit',      True),
	OPT_LINE_SEP  : ('L', 'line-sep',   True),
	OPT_NUMERIC   : ('n', 'numeric',    False),
	OPT_PASSWORD  : ('p', 'password',   True),
	OPT_REC_SEP   : ('R', 'record-sep', True),
	OPT_COL_SEP   : ('S', 'column-sep', True),
	OPT_TABLE     : ('t', 'table',      False),
	OPT_VERBOSE   : ('v', 'verbose',    False),
	OPT_QUIET     : ('q', 'quiet',      False),
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