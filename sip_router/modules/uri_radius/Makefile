# $Id: Makefile,v 1.4 2003/12/17 15:43:12 sobomax Exp $
#
# URI checks using Radius
#
# 
# WARNING: do not run this directly, it should be run by the master Makefile

include ../../Makefile.defs
auto_gen=
NAME=uri_radius.so
DEFS+=-I$(LOCALBASE)/include
LIBS=-L$(LOCALBASE)/lib -L/usr/pkg/lib -lradiusclient

include ../../Makefile.modules
