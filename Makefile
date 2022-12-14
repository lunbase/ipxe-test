# Makefile for The PCI Utilities
# (c) 1998--2017 Martin Mares <mj@ucw.cz>

ifeq ($(OS),Windows_NT)
    OS_TYPE := $(OS)
else
    OS_TYPE := $(shell uname -s)
endif

OPT=-O2 -g -s -fwrapv -fpie -pie -z noexecstack -Wl,-z,now -Wl,-z,relro,-z,now -D_FORTIFY_SOURCE=1 -fstack-protector-strong
CFLAGS=$(OPT) -Wall -W -Wno-parentheses -Wstrict-prototypes -Wmissing-prototypes

VERSION=3.6.8
DATE=2022-08-01

# Host OS and release (override if you are cross-compiling)
HOST=
RELEASE=
CROSS_COMPILE=

# Support for compressed pci.ids (yes/no, default: detect)
ZLIB=

# Support for resolving ID's by DNS (yes/no, default: detect)
DNS=

# Build libpci as a shared library (yes/no; or local for testing; requires GCC)
SHARED=no

# Use libkmod to resolve kernel modules on Linux (yes/no, default: detect)
LIBKMOD=

# Use libudev to resolve device names using hwdb on Linux (yes/no, default: detect)
HWDB=no

# ABI version suffix in the name of the shared library
# (as we use proper symbol versioning, this seldom needs changing)
ABI_VERSION=.3

ifdef HOST
ifndef CROSS_COMPILE
CROSS_COMPILE=$(HOST)-
endif
endif

# Installation directories
PREFIX=/usr/local
SBINDIR=$(PREFIX)/sbin
SHAREDIR=$(PREFIX)/share
IDSDIR=$(SHAREDIR)
MANDIR:=$(shell if [ -d $(PREFIX)/share/man ] ; then echo $(PREFIX)/share/man ; else echo $(PREFIX)/man ; fi)
INCDIR=$(PREFIX)/include
LIBDIR=$(PREFIX)/lib
PKGCFDIR=$(LIBDIR)/pkgconfig

# Commands
INSTALL=install
DIRINSTALL=install -d
STRIP=-s
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
AR=$(CROSS_COMPILE)ar
RANLIB=$(CROSS_COMPILE)ranlib

# Base name of the library (overriden on NetBSD, which has its own libpci)
LIBNAME=libpci

-include lib/config.mk

PCIINC=lib/config.h lib/header.h lib/pci.h lib/types.h lib/sysdep.h
PCIINC_INS=lib/config.h lib/header.h lib/pci.h lib/types.h

export

all: lib/$(PCILIB) wxtool$(EXEEXT) lspci$(EXEEXT) setpci$(EXEEXT) wavetool$(EXEEXT) pcitool$(EXEEXT) example$(EXEEXT) lspci.8 setpci.8 pcitool.8 pcilib.7 update-pciids update-pciids.8 $(PCI_IDS)

lib/$(PCILIB): $(PCIINC) force
	$(MAKE) -C lib all

force:

lib/config.h lib/config.mk:
	if [ "$(OS)" == Windows_NT ] ;then cp -af win32/* lib/; else cd lib && ./configure; fi

lspci$(EXEEXT): lspci.o ls-vpd.o ls-caps.o ls-caps-vendor.o ls-ecaps.o ls-kernel.o ls-tree.o ls-map.o common.o lib/$(PCILIB)
	$(CC) -o $@ $+ $(LDLIBS)
setpci$(EXEEXT): setpci.o common.o lib/$(PCILIB)
	$(CC) -o $@ $+ $(LDLIBS)
pcitool$(EXEEXT): pcitool.o common.o lib/$(PCILIB)
	$(CC) -o $@ $+ $(LDLIBS)


LSPCIINC=lspci.h pciutils.h $(PCIINC)
lspci.o: lspci.c $(LSPCIINC)
ls-vpd.o: ls-vpd.c $(LSPCIINC)
ls-caps.o: ls-caps.c $(LSPCIINC)
ls-ecaps.o: ls-ecaps.c $(LSPCIINC)
ls-kernel.o: ls-kernel.c $(LSPCIINC)
ls-tree.o: ls-tree.c $(LSPCIINC)
ls-map.o: ls-map.c $(LSPCIINC)

setpci.o: setpci.c pciutils.h $(PCIINC)
pcitool.o: pcitool.c pciutils.h $(PCIINC)
common.o: common.c pciutils.h $(PCIINC)

lspci: LDLIBS+=$(LIBKMOD_LIBS)
ls-kernel.o: CFLAGS+=$(LIBKMOD_CFLAGS)

update-pciids: update-pciids.sh
	sed <$< >$@ "s@^DEST=.*@DEST=$(IDSDIR)/$(PCI_IDS)@;s@^PCI_COMPRESSED_IDS=.*@PCI_COMPRESSED_IDS=$(PCI_COMPRESSED_IDS)@"
	chmod +x $@

# The example of use of libpci
example$(EXEEXT): example.o lib/$(PCILIB)
	$(CC) -o $@ $+ $(LDLIBS)
example.o: example.c $(PCIINC)


# wavetool
wavetool$(EXEEXT): wavetool.o common.o lib/$(PCILIB)
	$(CC) -o $@ $+ $(LDLIBS)
wavetool.o: wavetool.c $(PCIINC)
# flash_opt
wxtool$(EXEEXT): wxtool.o common.o ls-vpd.o ls-caps.o ls-caps-vendor.o ls-ecaps.o ls-kernel.o ls-tree.o ls-map.o lib/$(PCILIB)
	$(CC) $(CFLAGS) -o $@ $+ $(LDLIBS) -lm
wxtool.o: wxtool.c $(PCIINC)

%: %.o
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LDLIBS) -o $@

%.8 %.7: %.man
	M=`echo $(DATE) | sed 's/-01-/-January-/;s/-02-/-February-/;s/-03-/-March-/;s/-04-/-April-/;s/-05-/-May-/;s/-06-/-June-/;s/-07-/-July-/;s/-08-/-August-/;s/-09-/-September-/;s/-10-/-October-/;s/-11-/-November-/;s/-12-/-December-/;s/\(.*\)-\(.*\)-\(.*\)/\3 \2 \1/'` ; sed <$< >$@ "s/@TODAY@/$$M/;s/@VERSION@/pciutils-$(VERSION)/;s#@IDSDIR@#$(IDSDIR)#"

clean:
	rm -f `find . -name "*~" -o -name "*.[oa]" -o -name "\#*\#" -o -name TAGS -o -name core -o -name "*.orig"`
	rm -f update-pciids *.exe wxtool$(EXEEXT) lspci$(EXEEXT) setpci$(EXEEXT) pcitool$(EXEEXT) example$(EXEEXT) lib/config.* *.[78] pci.ids.* lib/*.pc lib/*.so lib/*.so.*
	rm -rf maint/dist

distclean: clean

install: all
# -c is ignored on Linux, but required on FreeBSD
	$(DIRINSTALL) -m 755 $(DESTDIR)$(SBINDIR) $(DESTDIR)$(IDSDIR) $(DESTDIR)$(MANDIR)/man8 $(DESTDIR)$(MANDIR)/man7
	$(INSTALL) -c -m 755 $(STRIP) lspci$(EXEEXT) setpci$(EXEEXT) pcitool$(EXEEXT) $(DESTDIR)$(SBINDIR)
	$(INSTALL) -c -m 755 update-pciids $(DESTDIR)$(SBINDIR)
	$(INSTALL) -c -m 644 $(PCI_IDS) $(DESTDIR)$(IDSDIR)
	$(INSTALL) -c -m 644 lspci.8 setpci.8 pcitool.8 update-pciids.8 $(DESTDIR)$(MANDIR)/man8
	$(INSTALL) -c -m 644 pcilib.7 $(DESTDIR)$(MANDIR)/man7
ifeq ($(SHARED),yes)
ifeq ($(LIBEXT),dylib)
	ln -sf $(PCILIB) $(DESTDIR)$(LIBDIR)/$(LIBNAME)$(ABI_VERSION).$(LIBEXT)
else
	ln -sf $(PCILIB) $(DESTDIR)$(LIBDIR)/$(LIBNAME).$(LIBEXT)$(ABI_VERSION)
endif
endif

ifeq ($(SHARED),yes)
install: install-pcilib
endif

install-pcilib: lib/$(PCILIB)
	$(DIRINSTALL) -m 755 $(DESTDIR)$(LIBDIR)
	$(INSTALL) -c -m 644 lib/$(PCILIB) $(DESTDIR)$(LIBDIR)

install-lib: $(PCIINC_INS) lib/$(PCILIBPC) install-pcilib
	$(DIRINSTALL) -m 755 $(DESTDIR)$(INCDIR)/pci $(DESTDIR)$(PKGCFDIR)
	$(INSTALL) -c -m 644 $(PCIINC_INS) $(DESTDIR)$(INCDIR)/pci
	$(INSTALL) -c -m 644 lib/$(PCILIBPC) $(DESTDIR)$(PKGCFDIR)
ifeq ($(SHARED),yes)
ifeq ($(LIBEXT),dylib)
	ln -sf $(LIBNAME)$(ABI_VERSION).$(LIBEXT) $(DESTDIR)$(LIBDIR)/$(LIBNAME).$(LIBEXT)
else
	ln -sf $(LIBNAME).$(LIBEXT)$(ABI_VERSION) $(DESTDIR)$(LIBDIR)/$(LIBNAME).$(LIBEXT)
endif
endif

uninstall: all
	rm -f $(DESTDIR)$(SBINDIR)/lspci$(EXEEXT) $(DESTDIR)$(SBINDIR)/setpci$(EXEEXT) $(DESTDIR)$(SBINDIR)/pcitool$(EXEEXT) $(DESTDIR)$(SBINDIR)/update-pciids
	rm -f $(DESTDIR)$(IDSDIR)/$(PCI_IDS)
	rm -f $(DESTDIR)$(MANDIR)/man8/lspci.8 $(DESTDIR)$(MANDIR)/man8/setpci.8 $(DESTDIR)$(MANDIR)/man8/pcitool.8 $(DESTDIR)$(MANDIR)/man8/update-pciids.8
	rm -f $(DESTDIR)$(MANDIR)/man7/pcilib.7
ifeq ($(SHARED),yes)
	rm -f $(DESTDIR)$(LIBDIR)/$(PCILIB) $(DESTDIR)$(LIBDIR)/$(LIBNAME).so$(ABI_VERSION)
endif

pci.ids.gz: pci.ids
	gzip -9n <$< >$@

.PHONY: all clean distclean install install-lib uninstall force
