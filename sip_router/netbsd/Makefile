# New ports collection makefile for:    ser
# Date created:        6 November 2002
# Whom:                andrei
#
# $FreeBSD$
#
# $Id: Makefile,v 1.1 2003/06/02 19:02:40 andrei Exp $
#


PORTNAME= ser
PORTVERSION= 0.8.11pre29
CATEGORIES= net
MASTER_SITES= ftp://ftp.berlios.de/pub/ser/0.8.11pre29/src/

MAINTAINER= pelinescu-onciul@fokus.fraunhofer.de

MAN8=ser.8
MAN5=ser.cfg.5
MANCOMPRESSED= no


USE_GMAKE= yes
# we override the DISTFILE NAME
DISTNAME= ${PORTNAME}-${PORTVERSION}_src
WRKSRC= ${WRKDIR}/${PORTNAME}-${PORTVERSION}

#.include "/usr/pkgsrc/mk/bsd.prefs.mk"

pre-fetch:
	@echo "DEBUG: distfile=${DISTFILE}, distname=${DISTNAME}, w=${WRKSRC}"

.include "../../mk/bsd.pkg.mk"


