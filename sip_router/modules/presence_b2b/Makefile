# $Id: Makefile,v 1.4 2008/06/05 16:36:02 andrei Exp $
#
# Registrar Presence User Agent
#
# 
# WARNING: do not run this directly, it should be run by the master Makefile

include ../../Makefile.defs
auto_gen=
NAME = presence_b2b.so


# if using libcds, the directive SER must be defined !
# and root ser directory must be in include directories

DEFS+=-DSER
INCLUDES += -I/usr/include/libxml2 -I$(LOCALBASE)/include/libxml2 \
			-I$(LOCALBASE)/include -I../../lib -I../..
LIBS += -L$(LOCALBASE)/lib -L/usr/pkg/lib -lxml2 
SERLIBPATH=../../lib
SER_LIBS=$(SERLIBPATH)/presence/ser_presence $(SERLIBPATH)/cds/ser_cds \
			
# not used: $(SERLIBPATH)/xcap/ser_xcap

include ../../Makefile.modules
