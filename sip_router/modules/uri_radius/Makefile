# $Id: Makefile,v 1.5 2005/03/07 20:01:30 janakj Exp $
#
# URI checks using Radius
#
# 
# WARNING: do not run this directly, it should be run by the master Makefile

include ../../Makefile.defs
auto_gen=
NAME=uri_radius.so
DEFS+=-I$(LOCALBASE)/include
LIBS=-L$(LOCALBASE)/lib -L/usr/pkg/lib -lradiusclient-ng

include ../../Makefile.modules
