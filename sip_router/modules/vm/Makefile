# $Id: Makefile,v 1.2 2003/04/17 11:16:21 jiri Exp $
#
# print example module makefile
#
# 
# WARNING: do not run this directly, it should be run by the master Makefile

auto_gen=
NAME=vm.so
LIBS=

ifeq ($(MAKELEVEL), 0)
# make called directly in the module dir!

all::
	cd ../..; make all

else

include ../../Makefile.modules

endif
