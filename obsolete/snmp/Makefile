# $Id: Makefile,v 1.4 2003/02/24 16:46:11 andrei Exp $
#
# WARNING: do not run this directly, it should be run by the master Makefile

auto_gen=
NAME=snmp.so

include ../../Makefile.modules

# Tried using net-snmp-config to generate the include paths according to the local 
# installation, but seems like their compile flags clash with our compile flags
# (this is in gcc 2.96 in redhat. in debian (both gcc 2.95.4 and gcc-3) works fine)
# The way it should be:
# CFLAGS += `net-snmp-config --cflags`
#
CFLAGS += -I$(LOCALBASE)/include
LIBS+=`net-snmp-config --agent-libs`
