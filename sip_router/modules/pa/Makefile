# $Id: Makefile,v 1.8 2005/10/27 10:58:25 kubartv Exp $
#
# Presence Agent
#
# 
# WARNING: do not run this directly, it should be run by the master Makefile

include ../../Makefile.defs
auto_gen=
NAME=pa.so
LIBS=

# DEFS+=-I/usr/include/libxml2 -I$(LOCALBASE)/include/libxml2
# LIBS+=-L/usr/include/lib  -L$(LOCALBASE)/lib -lxml2

# experimental (vku)
DEFS += -I/usr/include/libxml2 -I$(LOCALBASE)/include/libxml2 -Wall -I../../lib -DSER -I../..
LIBS += -L$(LOCALBASE)/lib -L/usr/pkg/lib -L../../lib/xcap -L../../lib/presence -L../../lib/cds -lxcap -lpresence -lcds -lxml2 -lcurl 

include ../../Makefile.modules
