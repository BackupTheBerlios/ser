# $Id: Makefile,v 1.10 2005/06/01 22:25:06 andrei Exp $
#
# example module makefile
#
# 
# WARNING: do not run this directly, it should be run by the master Makefile

include ../../Makefile.defs
auto_gen=
NAME=jabber.so

# extra debug messages
DEFS+=-DXJ_EXTRA_DEBUG
# -DHAVE_IHTTP

# expat.h location
DEFS+=-I$(LOCALBASE)/include
LIBS=-L$(LOCALBASE)/lib -lexpat

include ../../Makefile.modules
