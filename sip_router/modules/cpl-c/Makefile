# $Id: Makefile,v 1.5 2003/09/03 17:42:10 bogdan Exp $
#
# example module makefile
#
# 
# WARNING: do not run this directly, it should be run by the master Makefile

include ../../Makefile.defs
auto_gen=
NAME=cpl-c.so

DEFS +=-I/usr/include/libxml2

LIBS= -L$(LOCALBASE)/lib -L/usr/pkg/lib -lxml2

include ../../Makefile.modules
