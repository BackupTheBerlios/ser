# $Id: Makefile,v 1.3 2007/05/08 08:35:24 andrei Exp $
#
# Registrar Presence User Agent
#
# 
# WARNING: do not run this directly, it should be run by the master Makefile

include ../../Makefile.defs
auto_gen=
NAME = dialog.so

# if using libcds, the directive SER must be defined !
# and root ser directory must be in include directories

DEFS+=-DSER
INCLUDES = -I../../lib -I../..
LIBS = 
SERLIBPATH=../../lib
SER_LIBS=$(SERLIBPATH)/cds/ser_cds

include ../../Makefile.modules
