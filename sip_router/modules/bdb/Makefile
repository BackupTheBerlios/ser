# $Id: Makefile,v 1.1 2007/03/17 01:28:52 sobomax Exp $
#
# example module makefile
#
# 
# WARNING: do not run this directly, it should be run by the master Makefile

# extra debug messages
#DEFS+=-DBDB_EXTRA_DEBUG
 
include ../../Makefile.defs
auto_gen=
NAME=bdb.so

# db.h locations
#DEFS += -I$(LOCALBASE)/include/db41
#LIBS  = -L$(LOCALBASE)/lib -ldb41
DEFS += -I$(LOCALBASE)/include/db44
LIBS  = -L$(LOCALBASE)/lib -ldb-4.4


include ../../Makefile.modules
