#!/usr/bin/python2.3
#
# $Id: migrate_ser_db.py,v 1.1 2006/11/17 00:17:27 janakj Exp $
#
# Copyright (C) 2006 iptelorg GmbH
#
# This script is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version
#
# This script is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
import sys, string, os, getopt, MySQLdb, common

from domain import convert_domains
from subscribers import convert_subscribers
from uri import *
from ul import convert_ul
from cpl import convert_cpl
from msilo import convert_msilo
from phonebook import convert_phonebook
from admin_privileges import convert_admin_privileges
from grp import convert_grp
from usr_preferences import convert_usr_preferences
from acc import convert_acc
from parse_uri import parse

DEFAULT_INPUT_URI="mysql://ser:heslo@localhost/ser"
DEFAULT_OUTPUT_URI="mysql://ser:heslo@localhost/ser_new"


DEFAULT_TABLES = "domain,subscribers,pending,aliases,uri,location,cpl,msilo,phonebook,admin_privileges,grp,user_preferences,acc,missed_calls";


def printUsage():
    name = string.split(sys.argv[0],os.sep)[-1]
    sys.stderr.write("""
usage: %s [options]

Options:
  -h or --help
     Print this help message

  -v or --verbose
     Be verbose

  -i or --input
     Input database
     Default: %s

  -o or --output
     Destination (new) database
     Default: %s

  -t or --table
     Here you can specify comma separated list of tables to convert
     Default: %s
         
""" % (name, DEFAULT_INPUT_URI, DEFAULT_OUTPUT_URI, DEFAULT_TABLES))


def connect_db(uri):
    scheme, user, password, host, port, db = parse(uri);

    if not (scheme == "mysql"):
        print "ERROR: Unsupported database %s\n" % scheme
        sys.exit()

    params = []
    if user is not None:
        params.append("user='%s'" % user)
    if password is not None:
        params.append("passwd='%s'" % password)
    if host is not None:
        params.append("host='%s'" % host)
    if port is not None:
        params.append("port=%s" % port)
    if db is not None:
        params.append("db='%s'" % db)
    params = ', '.join(params)
    connect = 'MySQLdb.connect(%s)' % params
    return eval(connect)


def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hvi:o:t:", ["help", "verbose", "input=", "output=", "table="])
    except getopt.GetoptError:
        printUsage()
        sys.exit(2)
    output = None
    
    input_uri = DEFAULT_INPUT_URI
    output_uri = DEFAULT_OUTPUT_URI
    tables_str = DEFAULT_TABLES
    
    for o, a in opts:
        if o in ("-v", "--verbose"):
            common.verbose = True
        if o in ("-h", "--help"):
            printUsage()
            sys.exit()
        if o in ("-i", "--input"):
            input_uri = a
        if o in ("-o", "--output"):
            output_uri = a
        if o in ("-t", "--table"):
            tables_str = a

    if common.verbose is True:
        print "Using %s as the source database" % input_uri
        print "Using %s as the destination database" % output_uri

    try:
        src = connect_db(input_uri)
    except MySQLdb.OperationalError:
        print "ERROR while connecting database %s\n" % input_uri
        sys.exit(1)

    try:
        dst = connect_db(output_uri)
    except MySQLdb.OperationalError:
        print "ERROR while connecting database %s\n" % output_uri
        sys.exit(1)

    tables = tables_str.split(',')

    try:
        tables.index('domain')
        convert_domains(dst, src)
    except ValueError: pass

    try:
        tables.index('subscriber')
        convert_subscribers("subscriber", dst, src, 0)
        convert_uri_from_credentials(dst, src)
    except ValueError: pass

    try:
        tables.index('pending')
        convert_subscribers("pending", dst, src, common.DB_DISABLED)
    except ValueError: pass

    try:
        tables.index('aliases')
        convert_uri_from_aliases(dst, src)
    except ValueError: pass

    try:
        tables.index('uri')
        convert_uri_from_uri(dst, src)
    except ValueError: pass

    try:
        tables.index('location')
        convert_ul(dst, src)
    except ValueError: pass

    try:
        tables.index('cpl')
        convert_cpl(dst, src)
    except ValueError: pass

    try:
        tables.index('msilo')
        convert_msilo(dst, src)
    except ValueError: pass

    try:
        tables.index('phonebook')
        convert_phonebook(dst, src)
    except ValueError: pass

    try:
        tables.index('admin_privileges')
        convert_admin_privileges(dst, src)
    except ValueError: pass

    try:
        tables.index('grp')
        convert_grp(dst, src)
    except ValueError: pass

    try:
        tables.index('usr_preferences')
        convert_usr_preferences(dst, src)
    except ValueError: pass

    try:
        tables.index('acc')
        convert_acc("acc", dst, src)
    except ValueError: pass

    try:
        tables.index('missed_calls')
        convert_acc("missed_calls", dst, src)
    except ValueError: pass

    dst.close()
    src.close()

if __name__ == "__main__":
    main()
