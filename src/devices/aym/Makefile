library_name := devices_aym
path_step := ../../..
source_dirs := src

include $(path_step)/makefile.mak

ifndef NO_PERL
$(CURDIR)/src/device.h: $(CURDIR)/src/gen_aym.pl $(CURDIR)/src/device.h.templ
	perl $^ > $@
endif
