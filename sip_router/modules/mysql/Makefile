# $Id: Makefile,v 1.15 2002/11/12 12:32:17 andrei Exp $
#
# WARNING: do not run this directly, it should be run by the master Makefile

auto_gen=
NAME=mysql.so
# mysql.h locations (freebsd,openbsd  solaris)
DEFS +=-I/usr/local/include -I/usr/local/mysql/include \
# libmysqlclient locations on RH/Suse, Solaris /OpenBSD, FreeBSD
# (Debian does the right thing and puts it in /usr/lib)
LIBS=-L/usr/lib/mysql -L /usr/local/lib -L/usr/local/lib/mysql \
		-L/usr/local/mysql/lib/mysql/ \
		-lmysqlclient -lz 

include ../../Makefile.modules
