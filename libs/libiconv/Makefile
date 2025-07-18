# Makefile for libiconv

#### Start of system configuration section. ####

# Directories used by "make":
srcdir = .

# Directories used by "make install":
prefix = /opt/devkitpro-r24/portlibs/ppc
local_prefix = /usr/local
exec_prefix = ${prefix}
datarootdir = ${prefix}/share
datadir = ${datarootdir}
libdir = /opt/devkitpro-r24/portlibs/ppc/lib
includedir = ${prefix}/include
mandir = ${datarootdir}/man

# Programs used by "make":
RM = rm -f


# Programs used by "make install":
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
mkinstalldirs = $(SHELL) ./build-aux/mkinstalldirs

#### End of system configuration section. ####

SHELL = /bin/sh

all : lib/localcharset.h force
	cd lib && $(MAKE) all
	cd preload && $(MAKE) all
	cd srclib && $(MAKE) all
	cd src && $(MAKE) all
	cd po && $(MAKE) all
	cd man && $(MAKE) all
	if test -d tests; then cd tests && $(MAKE) all; fi

lib/localcharset.h :
	builddir="`pwd`"; cd libcharset && $(MAKE) all && $(MAKE) install-lib libdir="$$builddir/lib" includedir="$$builddir/lib"

# Installs the library and include files only. Typically called with only
# $(libdir) and $(includedir) - don't use $(prefix) and $(exec_prefix) here.
install-lib : all force
	cd lib && $(MAKE) install-lib libdir='$(libdir)' includedir='$(includedir)'
	if [ ! -d $(includedir) ] ; then $(mkinstalldirs) $(includedir) ; fi
	$(INSTALL_DATA) include/iconv.h.inst $(includedir)/iconv.h

install : lib/localcharset.h force
	cd libcharset && $(MAKE) install prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd lib && $(MAKE) install prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd preload && $(MAKE) install prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd srclib && $(MAKE) install prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd src && $(MAKE) install prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	if [ ! -d $(DESTDIR)$(includedir) ] ; then $(mkinstalldirs) $(DESTDIR)$(includedir) ; fi
	$(INSTALL_DATA) include/iconv.h.inst $(DESTDIR)$(includedir)/iconv.h
	cd po && $(MAKE) install prefix='$(prefix)' exec_prefix='$(exec_prefix)' datarootdir='$(datarootdir)' datadir='$(datadir)'
	cd man && $(MAKE) install prefix='$(prefix)' exec_prefix='$(exec_prefix)' datarootdir='$(datarootdir)' datadir='$(datadir)' mandir='$(mandir)'

install-strip : lib/localcharset.h force
	cd libcharset && $(MAKE) install-strip prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd lib && $(MAKE) install-strip prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd preload && $(MAKE) install-strip prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd srclib && $(MAKE) install-strip prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd src && $(MAKE) install-strip prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	if [ ! -d $(DESTDIR)$(includedir) ] ; then $(mkinstalldirs) $(DESTDIR)$(includedir) ; fi
	$(INSTALL_DATA) include/iconv.h.inst $(DESTDIR)$(includedir)/iconv.h
	cd po && $(MAKE) install-strip prefix='$(prefix)' exec_prefix='$(exec_prefix)' datarootdir='$(datarootdir)' datadir='$(datadir)'
	cd man && $(MAKE) install-strip prefix='$(prefix)' exec_prefix='$(exec_prefix)' datarootdir='$(datarootdir)' datadir='$(datadir)' mandir='$(mandir)'

installdirs : force
	cd libcharset && $(MAKE) installdirs prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd lib && $(MAKE) installdirs prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd preload && $(MAKE) installdirs prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd srclib && $(MAKE) installdirs prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd src && $(MAKE) installdirs prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	if [ ! -d $(DESTDIR)$(includedir) ] ; then $(mkinstalldirs) $(DESTDIR)$(includedir) ; fi
	cd po && $(MAKE) installdirs prefix='$(prefix)' exec_prefix='$(exec_prefix)' datarootdir='$(datarootdir)' datadir='$(datadir)'
	cd man && $(MAKE) installdirs prefix='$(prefix)' exec_prefix='$(exec_prefix)' datarootdir='$(datarootdir)' datadir='$(datadir)' mandir='$(mandir)'

uninstall : force
	cd libcharset && $(MAKE) uninstall prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd lib && $(MAKE) uninstall prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd preload && $(MAKE) uninstall prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd srclib && $(MAKE) uninstall prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	cd src && $(MAKE) uninstall prefix='$(prefix)' exec_prefix='$(exec_prefix)' libdir='$(libdir)'
	$(RM) $(DESTDIR)$(includedir)/iconv.h
	cd po && $(MAKE) uninstall prefix='$(prefix)' exec_prefix='$(exec_prefix)' datarootdir='$(datarootdir)' datadir='$(datadir)'
	cd man && $(MAKE) uninstall prefix='$(prefix)' exec_prefix='$(exec_prefix)' datarootdir='$(datarootdir)' datadir='$(datadir)' mandir='$(mandir)'

check : all force
	cd libcharset && $(MAKE) check
	cd lib && $(MAKE) check
	cd preload && $(MAKE) check
	cd srclib && $(MAKE) check
	cd src && $(MAKE) check
	cd po && $(MAKE) check
	cd man && $(MAKE) check
	if test -d tests; then cd tests && $(MAKE) check; fi

mostlyclean : force
	cd libcharset && $(MAKE) mostlyclean
	$(RM) lib/charset.alias lib/libcharset.* lib/localcharset.h
	cd lib && $(MAKE) mostlyclean
	cd preload && $(MAKE) mostlyclean
	cd srclib && $(MAKE) mostlyclean
	cd src && $(MAKE) mostlyclean
	cd po && $(MAKE) mostlyclean
	cd man && $(MAKE) mostlyclean
	if test -d tests; then cd tests && $(MAKE) mostlyclean; fi
	rm -rf bin

clean : force
	cd libcharset && $(MAKE) clean
	$(RM) lib/charset.alias lib/libcharset.* lib/localcharset.h
	cd lib && $(MAKE) clean
	cd preload && $(MAKE) clean
	cd srclib && $(MAKE) clean
	cd src && $(MAKE) clean
	cd po && $(MAKE) clean
	cd man && $(MAKE) clean
	if test -d tests; then cd tests && $(MAKE) clean; fi
	rm -rf bin

distclean : force
	cd libcharset && if test -f Makefile; then $(MAKE) distclean; fi
	$(RM) lib/charset.alias lib/libcharset.* lib/localcharset.h
	cd lib && if test -f Makefile; then $(MAKE) distclean; fi
	cd preload && if test -f Makefile; then $(MAKE) distclean; fi
	cd srclib && if test -f Makefile; then $(MAKE) distclean; fi
	cd src && if test -f Makefile; then $(MAKE) distclean; fi
	cd po && if test -f Makefile; then $(MAKE) distclean; fi
	cd man && if test -f Makefile; then $(MAKE) distclean; fi
	if test -d tests; then cd tests && if test -f Makefile; then $(MAKE) distclean; fi; fi
	$(RM) config.status config.log config.cache Makefile libtool
	$(RM) config.h stamp-h1 lib/stamp-h2
	$(RM) include/iconv.h include/iconv.h.inst
	rm -rf bin

maintainer-clean : force
	cd libcharset && if test -f Makefile; then $(MAKE) maintainer-clean; fi
	$(RM) lib/charset.alias lib/libcharset.* lib/localcharset.h
	cd lib && if test -f Makefile; then $(MAKE) maintainer-clean; fi
	cd preload && if test -f Makefile; then $(MAKE) maintainer-clean; fi
	cd srclib && if test -f Makefile; then $(MAKE) maintainer-clean; fi
	cd src && if test -f Makefile; then $(MAKE) maintainer-clean; fi
	cd po && if test -f Makefile; then $(MAKE) maintainer-clean; fi
	cd man && if test -f Makefile; then $(MAKE) maintainer-clean; fi
	if test -d tests; then cd tests && if test -f Makefile; then $(MAKE) maintainer-clean; fi; fi
	$(RM) config.status config.log config.cache Makefile libtool
	$(RM) config.h stamp-h1 lib/stamp-h2
	$(RM) include/iconv.h include/iconv.h.inst
	rm -rf bin

force :

# Work around an automake-1.8 bug.
am--refresh:
