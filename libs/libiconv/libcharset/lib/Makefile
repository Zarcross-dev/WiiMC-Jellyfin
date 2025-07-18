# Makefile for libcharset/lib

#### Start of system configuration section. ####

# Directories used by "make":
srcdir = .

# Directories used by "make install":
prefix = /opt/devkitpro-r24/portlibs/ppc
local_prefix = /usr/local
exec_prefix = ${prefix}
libdir = /opt/devkitpro-r24/portlibs/ppc/lib

# Programs used by "make":
CC = /opt/devkitpro-r24/devkitPPC/bin/powerpc-eabi-gcc
CFLAGS = -g -O2 -fvisibility=hidden
CPPFLAGS = 
LDFLAGS = 
INCLUDES = -I. -I$(srcdir) -I.. -I$(srcdir)/.. -I../include
# -DBUILDING_LIBCHARSET: Change expansion of LIBCHARSET_DLL_EXPORTED macro.
# -DBUILDING_DLL: Change expansion of RELOCATABLE_DLL_EXPORTED macro.
DEFS = -DLIBDIR=\"$(libdir)\" -DBUILDING_LIBCHARSET -DBUILDING_DLL \
-DENABLE_RELOCATABLE=1 -DIN_LIBRARY -DINSTALLDIR=\"$(libdir)\" -DNO_XMALLOC \
-Dset_relocation_prefix=libcharset_set_relocation_prefix \
-Drelocate=libcharset_relocate -DHAVE_CONFIG_H
LIBTOOL = /bin/bash $(top_builddir)/libtool
LIBTOOL_COMPILE = $(LIBTOOL) --mode=compile
LIBTOOL_LINK = $(LIBTOOL) --mode=link
LIBTOOL_INSTALL = $(LIBTOOL) --mode=install
LIBTOOL_UNINSTALL = $(LIBTOOL) --mode=uninstall
RM = rm -f

# Programs used by "make install":
INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644
mkinstalldirs = $(SHELL) ../build-aux/mkinstalldirs

#### End of system configuration section. ####

PACKAGE = 
VERSION = 

SHELL = /bin/sh

# Before making a release, change this according to the libtool documentation,
# section "Library interface versions".
LIBCHARSET_VERSION_INFO = 1:0:0

# Needed by $(LIBTOOL).
top_builddir = ..

SOURCES = $(srcdir)/localcharset.c $(srcdir)/relocatable.c

OBJECTS = localcharset.lo relocatable.lo

all : libcharset.la charset.alias ref-add.sed ref-del.sed

libcharset.la : $(OBJECTS)
	$(LIBTOOL_LINK) $(CC) $(LDFLAGS) $(CFLAGS) -o libcharset.la -rpath $(libdir) -version-info $(LIBCHARSET_VERSION_INFO) -no-undefined $(OBJECTS)

localcharset.lo : $(srcdir)/localcharset.c
	$(LIBTOOL_COMPILE) $(CC) $(INCLUDES) $(CFLAGS) $(CPPFLAGS) $(DEFS) -c $(srcdir)/localcharset.c

relocatable.lo : $(srcdir)/relocatable.c $(srcdir)/relocatable.h
	$(LIBTOOL_COMPILE) $(CC) $(INCLUDES) $(CFLAGS) $(CPPFLAGS) $(DEFS) -c $(srcdir)/relocatable.c

charset.alias: $(srcdir)/config.charset
	$(SHELL) $(srcdir)/config.charset 'powerpc-unknown-eabi' > t-$@
	mv t-$@ $@

ref-add.sed : $(srcdir)/ref-add.sin
	sed -e '/^#/d' -e 's/@''PACKAGE''@//g' $(srcdir)/ref-add.sin > t-$@
	mv t-$@ $@

ref-del.sed : $(srcdir)/ref-del.sin
	sed -e '/^#/d' -e 's/@''PACKAGE''@//g' $(srcdir)/ref-del.sin > t-$@
	mv t-$@ $@

# Installs the library and include files only. Typically called with only
# $(libdir) and $(includedir) - don't use $(prefix) and $(exec_prefix) here.
install-lib : all force
	$(mkinstalldirs) $(libdir)
	$(LIBTOOL_INSTALL) $(INSTALL_DATA) libcharset.la $(libdir)/libcharset.la
	test -f $(libdir)/charset.alias && orig=$(libdir)/charset.alias \
	                                || orig=charset.alias; \
	sed -f ref-add.sed $$orig > $(libdir)/t-charset.alias; \
	$(INSTALL_DATA) $(libdir)/t-charset.alias $(libdir)/charset.alias; \
	rm -f $(libdir)/t-charset.alias

# The following is needed in order to install a simple file in $(libdir)
# which is shared with other installed packages. We use a list of referencing
# packages so that "make uninstall" will remove the file if and only if it
# is not used by another installed package.
# On systems with glibc-2.1 or newer, the file is redundant, therefore we
# avoid installing it.

install : all force
	if test no = no; then \
	  case 'eabi' in \
	    darwin[56]*) \
	      need_charset_alias=true ;; \
	    darwin* | cygwin* | mingw* | pw32* | cegcc*) \
	      need_charset_alias=false ;; \
	    *) \
	      need_charset_alias=true ;; \
	  esac ; \
	else \
	  need_charset_alias=false ; \
	fi ; \
	$(mkinstalldirs) $(DESTDIR)$(libdir) ; \
	$(LIBTOOL_INSTALL) $(INSTALL_DATA) libcharset.la $(DESTDIR)$(libdir)/libcharset.la
	if test -f $(DESTDIR)$(libdir)/charset.alias; then \
	  sed -f ref-add.sed $(DESTDIR)$(libdir)/charset.alias > $(DESTDIR)$(libdir)/t-charset.alias; \
	  $(INSTALL_DATA) $(DESTDIR)$(libdir)/t-charset.alias $(DESTDIR)$(libdir)/charset.alias; \
	  rm -f $(DESTDIR)$(libdir)/t-charset.alias; \
	else \
	  if $$need_charset_alias; then \
	    sed -f ref-add.sed charset.alias > $(DESTDIR)$(libdir)/t-charset.alias; \
	    $(INSTALL_DATA) $(DESTDIR)$(libdir)/t-charset.alias $(DESTDIR)$(libdir)/charset.alias; \
	    rm -f $(DESTDIR)$(libdir)/t-charset.alias; \
	  fi ; \
	fi

install-strip : install

installdirs : force
	$(mkinstalldirs) $(DESTDIR)$(libdir)

uninstall : force
	$(LIBTOOL_UNINSTALL) $(RM) $(DESTDIR)$(libdir)/libcharset.la
	if test -f $(DESTDIR)$(libdir)/charset.alias; then \
	  sed -f ref-del.sed $(DESTDIR)$(libdir)/charset.alias > $(DESTDIR)$(libdir)/t-charset.alias; \
	  if grep '^# Packages using this file: $$' $(DESTDIR)$(libdir)/t-charset.alias > /dev/null; then \
	    rm -f $(DESTDIR)$(libdir)/charset.alias; \
	  else \
	    $(INSTALL_DATA) $(DESTDIR)$(libdir)/t-charset.alias $(DESTDIR)$(libdir)/charset.alias; \
	  fi; \
	  rm -f $(DESTDIR)$(libdir)/t-charset.alias; \
	fi

check : all

mostlyclean : clean

clean : force
	$(RM) *.o *.lo *.a *.la *.o.lock core *.stackdump charset.alias ref-add.sed ref-del.sed
	$(RM) -r .libs _libs

distclean : clean
	$(RM) config.status config.log config.cache Makefile libtool

maintainer-clean : distclean

force :

