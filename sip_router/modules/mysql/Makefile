# $Id: Makefile,v 1.12 2002/11/02 01:35:08 andrei Exp $
#
# WARNING: do not run this directly, it should be run by the master Makefile

auto_gen=
NAME=mysql.so
DEFS +=-I/usr/local/mysql/include
# libmysqlclient locations on RH/Suse and Solaris
# (Debian does the right thing and puts it in /usr/lib)
LIBS=-L/usr/lib/mysql -L/usr/local/mysql/lib/mysql/ -lmysqlclient -lz 

include ../../Makefile.modules
