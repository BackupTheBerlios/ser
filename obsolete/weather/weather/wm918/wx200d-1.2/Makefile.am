## Process this file with automake to produce Makefile.in

# @(#)$Id: Makefile.am,v 1.1 2002/09/23 19:12:51 bogdan Rel $

# Copyright (C) 1998 - 2002 Tim Witham <twitham@quiknet.com>

# (see the files README and COPYING for more details)

# Look at ./configure --help before tweaking this directly.

man_MANS = wxplot.1 wxstdout.1 wxdebug.1 wxfilter.1 wx200.1 wx200d.8	\
wx200d.5

include_HEADERS = wx200.h

bin_PROGRAMS = wx200 wxstdout wxfilter wxdebug wxread
bin_SCRIPTS = wxplot

sbin_PROGRAMS = wx200d
sbin_SCRIPTS = wxsum wxplot2html wunderground wxrrdupdate wxrrdgraph	\
wxaprs wxindex wxvws

lib_LIBRARIES = libwx200.a

EXTRA_DIST = $(man_MANS) gstripchart.conf Makefile.data.in wx200.txt	\
wmr918.txt wmr918.TODO wxaprs rrdtool.gif

libwx200_a_SOURCES = client.c buffer.c parse.c unit.c format.c tab.c	\
parse.h wx200.h
wxread_SOURCES = wxread.c serial.c serial.h

d_SOURCES = wx200d.c serial.c serial.h wmr918util.c wmr918.h parse.h
if POSTGRES
wx200d_SOURCES = $(d_SOURCES) pg_api.c pg_api.h
else
wx200d_SOURCES = $(d_SOURCES)
endif
EXTRA_wx200d_SOURCES = pg_api.c pg_api.h

wx200_LDADD = -L. -lwx200
wxstdout_LDADD = $(wx200_LDADD)
wxdebug_LDADD = $(wx200_LDADD)
wxfilter_LDADD = $(wx200_LDADD)
wx200d_LDADD = $(wx200_LDADD)

wx200_DEPENDENCIES = libwx200.a
wxstdout_DEPENDENCIES = $(wx200_DEPENDENCIES)
wxdebug_DEPENDENCIES = $(wx200_DEPENDENCIES)
wxfilter_DEPENDENCIES = $(wx200_DEPENDENCIES)
wx200d_DEPENDENCIES = $(wx200_DEPENDENCIES)

pkgdata_DATA = Makefile.data
htmldir = $(pkgdatadir)/html
html_DATA = Makefile.data rrdtool.gif

# this is a hack but how else do you install an actual Makefile with automake?
install-data-local: $(pkgdata_DATA)
	mv $(DESTDIR)$(pkgdatadir)/Makefile.data $(DESTDIR)$(pkgdatadir)/Makefile
	mv $(DESTDIR)$(htmldir)/Makefile.data $(DESTDIR)$(htmldir)/Makefile
