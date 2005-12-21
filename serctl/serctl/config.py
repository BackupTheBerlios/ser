#
# $Id: config.py,v 1.1 2005/12/21 18:18:30 janakj Exp $
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
# Name of environment variable used to pass the database
# URI to serctl
#
ENV_DB = 'SERCTL_DB'

#
# Global contstants, do not touch the lines below unless
# you know what you are doing 
#

WHITESP  = ' \t'
REC_SEP  = ' '
LINE_SEP = '\n'
COL_SEP  = ','
