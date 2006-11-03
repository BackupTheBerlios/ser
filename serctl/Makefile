ver           = $(shell head -n 1 ChangeLog | sed -e 's/.*(//;s/).*//')
python_ver    = 2.4

name          = serctl
pkg_name      = $(name)-$(ver)

prefix        = 
usr_prefix    = $(prefix)/usr/local
etc_prefix    = $(prefix)/usr/local
var_prefix    = $(prefix)/var/local

sysconfdir    = $(etc_prefix)/etc
bindir        = $(usr_prefix)/bin
libexecdir    = $(usr_prefix)/lib
datadir       = $(usr_prefix)/share
docdir        = $(datadir)/doc
localstatedir = $(var_prefix)
mandir        = $(datadir)/man

PYPKG_DIR     = $(libexecdir)/python$(python_ver)/site-packages/$(name)
DOC_DIR       = $(docdir)/$(name)
MAN1_DIR      = $(mandir)/man1
MAN3_DIR      = $(mandir)/man3
PRECOMP_DIR   = serctl
CONF_DIR      = $(sysconfdir)/$(name)

# ------------------------------------------------------------------------- #

SRC_BIN_FILES  = ser_* 
SRC_DOC_FILES  = README TODO ChangeLog BUGS
SRC_CONF_FILES = serctl.conf
SRC_MAN1_FILES = man/{serctl.1,ser_cred.1,ser_domain.1,ser_ctl.1,ser_uri.1,ser_user.1}
SRC_MAN3_FILES = man/serxmlrpc.3

# ========================================================================= #

all: build precompile

build: doc
	echo "version = '$(ver)'" > serctl/version.py

precompile: build
	python    -c 'import py_compile; py_compile.main()' \
		`find $$PRECOMP_DIR -name '*.py'`
	python -O -c 'import py_compile; py_compile.main()' \
		`find $$PRECOMP_DIR -name '*.py'`

doc:
	cd man && make

i:
	make install-precomp prefix=install

install-precomp: install
	make precompile PRECOMP_DIR=$(prefix)

install: install-lib install-conf install-bin install-doc

install-lib: build
	# python package
	mkdir -m 0755 -p $(PYPKG_DIR)
	find serctl -type d ! -path '*CVS*' -printf "$(PYPKG_DIR)/%P\n" | xargs -n1 mkdir -p
	find serctl -type f ! -path '*CVS*' -name '*.py*' -printf "%p $(PYPKG_DIR)/%P\n" | xargs -n2 cp -a
	rm -f $(PYPKG_DIR)/localconfig.py

install-conf: build
	# config file(s)
	mkdir -m 0755 -p $(CONF_DIR)
	cp -a $(SRC_CONF_FILES) $(CONF_DIR)

install-bin: build
	# python bins
	mkdir -m 0755 -p $(bindir)
	cp -a $(SRC_BIN_FILES) $(bindir)

install-doc: build
	# documentation
	mkdir -m 0755 -p $(DOC_DIR)
	cp -a $(SRC_DOC_FILES) $(DOC_DIR)
	chmod 0644 $(DOC_DIR)/*
	mkdir -m 0755 -p $(MAN1_DIR)
	mkdir -m 0755 -p $(MAN3_DIR)
	cp -a $(SRC_MAN1_FILES) $(MAN1_DIR)
	cp -a $(SRC_MAN3_FILES) $(MAN3_DIR)

clean:
	find . -name '*~' -type f -exec rm -f \{\} \;
	find $(PRECOMP_DIR) -name '*.pyc' -type f -exec rm -f \{\} \;
	find $(PRECOMP_DIR) -name '*.pyo' -type f -exec rm -f \{\} \;

cvsclean:
	find . -depth -name 'CVS' -type d -exec rm -rf \{\} \;
	find . -name '.cvsignore' -type f -exec rm -f \{\} \;

nuke: clean
	rm -rf install $(pkg_name)
	rm -f $(pkg_name).tar.bz2
	rm -f debian/files *.deb *.dsc *.tar.gz *.changes
	cd man && make nuke

tree:
	rm -rf $(pkg_name)
	mkdir  $(pkg_name)
	tar c --exclude=$(pkg_name) . | tar -C $(pkg_name) -x
	cd $(pkg_name) && $(MAKE) nuke

tarball: tree
	tar -cjf $(pkg_name).tar.bz2 $(pkg_name)
	rm -r $(pkg_name)

deb: tree
	cd $(pkg_name) && $(MAKE) cvsclean
	cd $(pkg_name) && dpkg-buildpackage -rfakeroot -us -uc
	rm -rf $(pkg_name)

version:
	@echo $(ver)

.PHONY: all build doc precompile i install install-lib install-conf 
.PHONY: install-bin install-doc cleannuke tree tarball version
.PHONY: cvsclean
