#
# ldap module makefile
#
# 
# WARNING: do not run this directly, it should be run by the master Makefile
#

include ../../Makefile.defs
auto_gen=../auth_db/rfc2617.c
NAME=ldap.so
LIBS=-L/usr/local/lib -lssl -lcrypto -lcrypt -lldap -llber
CFLAGS=-Wall -g
DEFS+= -I/usr/local/include

include ../../Makefile.modules
