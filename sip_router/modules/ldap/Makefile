# $Id: Makefile,v 1.3 2008/07/18 10:02:49 gkovacs Exp $
#
# WARNING: do not run this directly, it should be run by the master Makefile

include ../../Makefile.defs
auto_gen=
NAME=ldap.so

#DEFS += -DLD_TEST

DEFS += -Wall -DOPENLDAP23

# Uncomment this if you have a recent version of libldap with
# LD_SCOPE_CHILDREN defined
#DEFS += -DHAVE_SCOPE_CHILDREN

LIBS=-L$(LOCALBASE)/lib -L /usr/lib -lldap

include ../../Makefile.modules
