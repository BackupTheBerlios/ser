#!/usr/bin/env python
# -*- encoding: UTF-8 -*-
#
# $Id: ctlhelp.py,v 1.5 2006/02/15 12:36:11 hallik Exp $
#
# Copyright (C) 2005 iptelorg GmbH
#
# This is part of SER (SIP Express Router), a free SIP server project. 
# You can redistribute it and/or modify it under the terms of GNU General
# Public License as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#

def main(args, opts):
	print help()

def options(args, opts):
	return """\
Options:
	-b --database    : Database URI,
	-c --config-file : Path to config file,
	-C --columns     : Show only specified columns,
	-d --disable     : Enable,
	-e --enable      : Disable,
	-f --force       : Ignore warnings, silent operations,
	-F --flags       : Numeric or symbolic flags,
	-g --debug       : Switch on python backtrace listing,
	-h --help        : This text,
	-l --limit       : Show only limited number of records,
	-L --line-sep    : Line separator (for show command),
	-n --numeric     : Use numeric (raw) - not symbolic show,
	-p --password    : Password,
	-R --record-sep  : Record separator (for show command),
	-s --ser-uri     : Ser uri for xmlrpc operations,
	-S --column-sep  : Column separator for -c option, (default is comma),
	-t --table       : Show output as table,
"""	

def help(args, opts):
	return """\
Usage:
	ctlser     <module_name> [options...] [--] [[command] params...]
	ser_ctl    [options...] [--] [[command] params...]
	ser_cred   [options...] [--] [[command] params...]
	ser_db     [options...] [--] [[command] params...]
	ser_domain [options...] [--] [[command] params...]
	ser_uri    [options...] [--] [[command] params...]
	ser_user   [options...] [--] [[command] params...]

%s
Commands:
	add, canonical, change, enable, disable, purge, rm, show

Use 'ser_<module_name> -h' or 'ctlser <module_name> -h' for module params specification.
""" % options(args, opts)
