prefix := /usr/local
srcdir := $(dir $(lastword $(MAKEFILE_LIST)))
bindir = $(prefix)/bin
libdir = $(prefix)/lib$(shell test -d $(prefix)/lib64 -a `uname -i` = x86_64 && echo 64)
CC = $(DO)gcc -s -O2 -Wall -o $@ $(if $(filter %.so,$@),-shared -fPIC) $(INCLUDEOPTS) $(filter %.c,$^)
AR = $(DO)ar rc $@ $^
CITO = $(DO)cito -o $@ $(patsubst %,-I %,$(sort $(dir $(filter-out %.ci,$^)))) $(filter %.ci,$^)
INSTALL = install
INSTALL_PROGRAM = mkdir -p $(DESTDIR)$(2) && $(INSTALL) $(1) $(DESTDIR)$(2)/$(or $(3),$(1))
INSTALL_DATA = mkdir -p $(DESTDIR)$(2) && $(INSTALL) -m 644 $(1) $(DESTDIR)$(2)/$(1)
ASCIIDOC = $(DO)asciidoc -o - $(1) $< | sed -e "s/527bbd;/c02020;/" | xmllint --valid --nonet -o $@ -
SDL_CFLAGS = `sdl-config --cflags`
SDL_LIBS = `sdl-config --libs`
SEVENZIP = 7z a -mx=9 -bd
MAKEZIP = $(DO)$(RM) $@ && $(SEVENZIP) -tzip $@ $(^:%=./%) # "./" makes 7z don't store paths in the archive
COPY = $(DO)cp $< $@
XASM = $(DO)xasm -q -o $@ $<
OSX_CC = $(DO)gcc -O2 -Wall -o $@ -undefined dynamic_lookup -arch x86_64 -arch i386 -mmacosx-version-min=10.4 $(INCLUDEOPTS) $(filter %.c,$^)

# no user-configurable paths below this line

MAKEFLAGS = -r
ifdef V
DO = mkdir -p $(@D) && 
else
DO = @echo $@ && mkdir -p $(@D) && 
endif
src = $(addprefix $(srcdir),$(1:.[ch]=.c) $(patsubst %.[ch],%.h,$(filter %.[ch],$(1))))
INCLUDEOPTS = $(patsubst %/,-I%,$(sort $(dir $(filter %.h,$^))))
CLEAN :=
CLEANDIR :=

all: asapconv libasap.a
.PHONY: all

install: install-asapconv install-lib
.PHONY: install

include $(srcdir)6502/6502.mk
include $(srcdir)www/www.mk
include $(srcdir)release/release.mk
include $(srcdir)test/test.mk

# asapconv

asapconv: $(call src,asapconv.c asap.[ch])
	$(CC)
CLEAN += asapconv

install-asapconv: asapconv
	$(call INSTALL_PROGRAM,asapconv,$(bindir))
.PHONY: install-asapconv

uninstall-asapconv:
	$(RM) $(DESTDIR)$(bindir)/asapconv
.PHONY: uninstall-asapconv

# lib

lib: libasap.a
.PHONY: lib

libasap.a: asap.o
	$(AR)
CLEAN += libasap.a

asap.o: $(call src,asap.[ch])
	$(CC) -c
CLEAN += asap.o

install-lib: libasap.a $(srcdir)asap.h
	$(call INSTALL_DATA,$(srcdir)asap.h,$(prefix)/include)
	$(call INSTALL_DATA,libasap.a,$(libdir))
.PHONY: install-lib

uninstall-lib:
	$(RM) $(DESTDIR)$(prefix)/include/asap.h $(DESTDIR)$(libdir)/libasap.a
.PHONY: uninstall-lib

# SDL

asap-sdl: $(call src,asap-sdl.c asap.[ch])
	$(CC) $(SDL_CFLAGS) $(SDL_LIBS)
CLEAN += asap-sdl

install-sdl: asap-sdl
	$(call INSTALL_PROGRAM,asap-sdl,$(bindir))
.PHONY: install-sdl

uninstall-sdl:
	$(RM) $(DESTDIR)$(bindir)/asap-sdl
.PHONY: uninstall-sdl

# asapscan

asapscan: $(srcdir)asapscan.c asap-asapscan.h
	$(CC)
CLEAN += asapscan asapscan.exe

asap-asapscan.h: $(call src,asap.ci asap6502.ci asapinfo.ci cpu6502.ci pokey.ci) $(ASM6502_PLAYERS_OBX) | asap-asapscan.c

asap-asapscan.c: $(call src,asap.ci asap6502.ci asapinfo.ci cpu6502.ci pokey.ci) $(ASM6502_PLAYERS_OBX)
	$(CITO) -D ASAPSCAN
CLEAN += asap-asapscan.c asap-asapscan.h

# asap.[ch]

$(srcdir)asap.h: $(call src,asap.ci asap6502.ci asapinfo.ci asapwriter.ci cpu6502.ci flashpack.ci pokey.ci) $(ASM6502_OBX) | $(srcdir)asap.c

$(srcdir)asap.c: $(call src,asap.ci asap6502.ci asapinfo.ci asapwriter.ci cpu6502.ci flashpack.ci pokey.ci) $(ASM6502_OBX)
	$(CITO) -D C -D FLASHPACK

# aatr.[ch]

$(srcdir)aatr.h: $(srcdir)aatr.ci | $(srcdir)aatr.c

$(srcdir)aatr.c: $(srcdir)aatr.ci
	$(CITO)

# other

$(srcdir)README.html: $(call src,README INSTALL CREDITS)
	$(call ASCIIDOC,-a toc -a asapsrc -a asapports)

sap-format.html: $(srcdir)sap-format.txt
	$(call ASCIIDOC,-a toc)

clean:
	$(RM) $(CLEAN)
	$(RM) -r $(CLEANDIR)
.PHONY: clean

.DELETE_ON_ERROR:

include $(srcdir)audacious/audacious.mk
include $(srcdir)gstreamer/gstreamer.mk
include $(srcdir)moc/moc.mk
include $(srcdir)vlc/vlc.mk
include $(srcdir)xbmc/xbmc.mk
include $(srcdir)xmms/xmms.mk

include $(srcdir)csharp/csharp.mk
include $(srcdir)d/d.mk
include $(srcdir)flash/flash.mk
include $(srcdir)java/java.mk
include $(srcdir)javascript/javascript.mk
include $(srcdir)perl/perl.mk
include $(srcdir)win32/win32.mk
