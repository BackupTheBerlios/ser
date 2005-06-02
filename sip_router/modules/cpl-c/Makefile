# $Id: Makefile,v 1.9 2005/06/02 23:59:41 andrei Exp $
#
# example module makefile
#
# 
# WARNING: do not run this directly, it should be run by the master Makefile

include ../../Makefile.defs
auto_gen=
NAME=cpl-c.so

DEFS +=-I/usr/include/libxml2 -I$(LOCALBASE)/include/libxml2 \
	-I$(LOCALBASE)/include  # iconv.h

LIBS= -L$(LOCALBASE)/lib -lxml2

include ../../Makefile.modules
