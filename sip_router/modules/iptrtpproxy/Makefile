# $Id: Makefile,v 1.2 2011/01/20 12:02:32 tma0 Exp $
#
# print example module makefile
#
# 
# WARNING: do not run this directly, it should be run by the master Makefile
#

# define path to netfilter/iptables directory
iptdir = 
include ../../Makefile.defs
auto_gen=
NAME=iptrtpproxy.so
LIBS += $(iptdir)/iptables.o -L $(iptdir)/libiptc -liptc -L $(iptdir)/extensions -lext
DEFS += -I $(iptdir)/include -I $(iptdir)/../../netfilter/iptables/include/

include ../../Makefile.modules

