ver           = $(shell head -n 1 ChangeLog | sed -e 's/.*(//;s/).*//')
python_ver    = 2.3

name          = serctl
pkg_name      = $(name)-$(ver)

prefix        = 
usr_prefix    = $(prefix)/usr/local
var_prefix    = $(prefix)/var/local

sysconfdir    = $(usr_prefix)/etc
bindir        = $(usr_prefix)/bin
libexecdir    = $(usr_prefix)/lib
datadir       = $(usr_prefix)/share
docdir        = $(datadir)/doc
localstatedir = $(var_prefix)

PYPKG_DIR     = $(libexecdir)/python$(python_ver)/site-packages/$(name)
DOC_DIR       = $(docdir)/$(name)
PRECOMP_DIR   = serctl

# ------------------------------------------------------------------------- #

SRC_DOC_FILES = README TODO ChangeLog

# ========================================================================= #

all: build precompile

build:

precompile:
	python    -c 'import py_compile; py_compile.main()' \
		`find $$PRECOMP_DIR -name '*.py'`
	python -O -c 'import py_compile; py_compile.main()' \
		`find $$PRECOMP_DIR -name '*.py'`

i:
	make install-precomp prefix=install

install-precomp: install
	make precompile PRECOMP_DIR=$(prefix)

install: install-lib install-bin install-doc

install-lib: build
	# python package
	mkdir -m 0755 -p $(PYPKG_DIR)
	find serctl -type d ! -path '*CVS*' -printf "$(PYPKG_DIR)/%P\n" | xargs -n1 mkdir -p
	find serctl -type f ! -path '*CVS*' -name '*.py*' -printf "%p $(PYPKG_DIR)/%P\n" | xargs -n2 cp -a

install-bin: build
	# python bins
	mkdir -m 0755 -p $(bindir)
	cp -a ser_* $(bindir)

install-doc: build
	# documentation
	mkdir -m 0755 -p $(DOC_DIR)
	cp -a $(SRC_DOC_FILES) $(DOC_DIR)
	chmod 0644 $(DOC_DIR)/*

clean:
	find . -name '*~' -type f -exec rm -f \{\} \;
	find $(PRECOMP_DIR) -name '*.pyc' -type f -exec rm -f \{\} \;
	find $(PRECOMP_DIR) -name '*.pyo' -type f -exec rm -f \{\} \;

nuke: clean
	rm -rf install $(pkg_name)
	rm -f $(pkg_name).tar.bz2

tree:
	rm -rf $(pkg_name)
	mkdir  $(pkg_name)
	tar c --exclude=$(pkg_name) . | tar -C $(pkg_name) -x
	cd $(pkg_name) && $(MAKE) nuke

tarball: tree
	tar -cjf $(pkg_name).tar.bz2 $(pkg_name)
	rm -r $(pkg_name)

version:
	@echo $(ver)

.PHONY: all build precompile i install install-lib install-bin install-doc clean
.PHONY: nuke tree tarball version
