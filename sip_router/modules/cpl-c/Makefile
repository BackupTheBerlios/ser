# $Id: Makefile,v 1.8 2005/06/01 22:25:05 andrei Exp $
#
# example module makefile
#
# 
# WARNING: do not run this directly, it should be run by the master Makefile

include ../../Makefile.defs
auto_gen=
NAME=cpl-c.so

DEFS +=-I/usr/include/libxml2 -I$(LOCALBASE)/include/libxml2

LIBS= -L$(LOCALBASE)/lib -lxml2

include ../../Makefile.modules
