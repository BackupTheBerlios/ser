#
# $Id: config.py,v 1.4 2006/01/12 17:07:18 hallik Exp $
#
# serctl configuration file
#

#
# Disable debugging mode
#
DEBUG = False

#
# Visible name of the tool
#
NAME  = "serctl"

#
# Verbosity level
#
VERB  = 1

#
# Database URI, this should be the SER database
#
DB_URI = 'mysql://ser:heslo@localhost/ser'

#
# Comment out the previous line and uncomment the line
# below to connect to postgresql based database
#
# DB_URI = 'postgres://ser:heslo@localhost/ser'

#
# Name of environment variable used to pass the database
# URI to serctl
#
ENV_DB = 'SERCTL_DB'

#
# Ser URI, this should be the SER URI for xmlrpc requests.
#
SER_URI = 'http://localhost:5060/'

#
# Name of environment variable used to pass the SER  URI 
# to serctl
#
ENV_SER = 'SERCTL_SER'

#
# Global contstants, do not touch the lines below unless
# you know what you are doing 
#

WHITESP  = ' \t'
REC_SEP  = ' '
LINE_SEP = '\n'
COL_SEP  = ','
