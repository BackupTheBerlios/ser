ver      = $(shell cd .. && make -s --no-print-directory version)
name_ver = serctl $(ver)

# ========================================================================= #

MANS = serctl.1 ser_ctl.1 ser_user.1 ser_domain.1 ser_cred.1 ser_uri.1

# ========================================================================= #

all: $(MANS)

%.1: %.1.pod
	name=`basename $@ .1 | tr a-z A-Z`; \
	pod2man -s 1 -n "$$name" -c 'Sip Express Router Control Tool' -r "$(name_ver)" $< $@ 

%.3: %.3.pod
	name=`basename $@ .3 | tr a-z A-Z`; \
	pod2man -s 3 -n "$$name" -c 'Sip Express Router Control Tool' -r "$(name_ver)" $< $@ 

clean:
	rm -f *.1 *.3

nuke: clean

.PHONY: all clean nuke
