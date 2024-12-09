#
# Makefile for libulz (stolen from musl) (requires GNU make)
#
# Use config.mak to override any of the following variables.
# Do not make changes here.
#

exec_prefix = /usr/local
bindir = $(exec_prefix)/bin

prefix = /usr/local/
includedir = $(prefix)/include
libdir = $(prefix)/lib


FLASH_SRCS = $(sort $(wildcard flashlib/*.c))
CORE_SRCS = $(sort $(wildcard neoart/flod/core/*.c))
BACKEND_SRCS = $(sort $(wildcard backends/*.c))

TRACKER_SRCS = $(sort $(wildcard neoart/flod/trackers/*.c))
FASTTRACKER_SRCS = $(sort $(wildcard neoart/flod/fasttracker/*.c))
WHITTAKER_SRCS = $(sort $(wildcard neoart/flod/whittaker/*.c))
FUTURECOMPOSER_SRCS = $(sort $(wildcard neoart/flod/futurecomposer/*.c))
DIGITALMUGICIAN_SRCS = $(sort $(wildcard neoart/flod/digitalmugician/*.c))
SIDMON_SRCS = $(sort $(wildcard neoart/flod/sidmon/*.c))
SOUNDFX_SRCS = $(sort $(wildcard neoart/flod/soundfx/*.c))
BPSOUNDMON_SRCS = $(sort $(wildcard neoart/flod/soundmon/*.c))
HUBBARD_SRCS = $(sort $(wildcard neoart/flod/hubbard/*.c))
FREDED_SRCS = $(sort $(wildcard neoart/flod/fred/*.c))
HIPPEL_SRCS = $(sort $(wildcard neoart/flod/hippel/*.c))
DELTA_SRCS = $(sort $(wildcard neoart/flod/deltamusic/*.c))

ALL_PLAYER_SRCS = $(WHITTAKER_SRCS) $(FUTURECOMPOSER_SRCS) $(TRACKER_SRCS) \
$(FASTTRACKER_SRCS) $(DIGITALMUGICIAN_SRCS) $(SIDMON_SRCS) $(SOUNDFX_SRCS) \
$(BPSOUNDMON_SRCS) $(HUBBARD_SRCS) $(FREDED_SRCS) $(HIPPEL_SRCS) $(DELTA_SRCS)

#FILELOADER_SRCS = neoart/flod/FileLoader.c
#PLAYER_SRCS = demos/Demo5.c
PLAYER_SRCS = $(sort $(wildcard player/*.c))

SRCS = $(BACKEND_SRCS) $(FLASH_SRCS) $(CORE_SRCS) $(ALL_PLAYER_SRCS) \
$(FILELOADER_SRCS) $(PLAYER_SRCS)

OBJS = $(SRCS:.c=.o)

CFLAGS += -O0 -g 
AR      = $(CROSS_COMPILE)ar
RANLIB  = $(CROSS_COMPILE)ranlib
OBJCOPY = $(CROSS_COMPILE)objcopy

#ALL_INCLUDES = $(sort $(wildcard include/*.h include/*/*.h))

#ALL_LIBS = libflod.a 
ALL_TOOLS = flodplayer
#ALL_TOOLS = flod_demo.out

-include config.mak

all: $(ALL_LIBS) $(ALL_TOOLS)

install: $(ALL_LIBS:lib/%=$(DESTDIR)$(libdir)/%) $(ALL_INCLUDES:include/%=$(DESTDIR)$(includedir)/%) $(ALL_TOOLS:tools/%=$(DESTDIR)$(bindir)/%)

clean:
	rm -f $(OBJS)
	rm -f $(ALL_TOOLS)
#	rm -f $(LOBJS)
#	rm -f $(ALL_LIBS) lib/*.[ao] lib/*.so

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

flodplayer: $(OBJS)
	$(CC) -o $@ $(OBJS) -lm -lao $(LDFLAGS)

flod_demo.out: $(OBJS)
	$(CC) -o $@ $(OBJS) -lm -lao $(LDFLAGS)

libflod.a: $(OBJS)
	rm -f $@
	$(AR) rc $@ $(OBJS)
	$(RANLIB) $@

lib/%.o:
	cp $< $@

$(DESTDIR)$(bindir)/%: tools/%
	install -D $< $@

$(DESTDIR)$(prefix)/%: %
	install -D -m 644 $< $@

.PHONY: all clean install
