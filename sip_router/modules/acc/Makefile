# $Id: Makefile,v 1.19 2004/02/24 10:39:45 dcm Exp $
#
# acc module makefile
#
#
# WARNING: do not run this directly, it should be run by the master Makefile

include ../../Makefile.defs
auto_gen=
NAME=acc.so
LIBS=

# uncomment the next line if you wish to enable SQL accounting
#DEFS+=-DSQL_ACC

# uncomment the next two lines if you wish to enable RADIUS accounting
#DEFS+=-DRAD_ACC -I$(LOCALBASE)/include
#LIBS=-L$(LOCALBASE)/lib -lradiusclient

# uncomment the next two lines if you wish to enable DIAMETER accounting
#DEFS+=-DDIAM_ACC

include ../../Makefile.modules
