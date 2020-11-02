library_name = fake
dirs.root = ../..
include $(dirs.root)/make/default.mak

inputs := $(wildcard *.spc)

all: $(patsubst %.spc,%.info,$(inputs)) $(patsubst %.spc,%.dump,$(inputs))

include $(dirs.root)/makefile.mak

modinfo = $(bins_dir)/$(call makebin_name,module_info)
dumper = $(bins_dir)/$(call makebin_name,formats_test_spc)

$(modinfo):
	$(MAKE) -C $(dirs.root)/src/core/test/modinfo

$(dumper):
	$(MAKE) -C $(dirs.root)/src/formats/test/dumpers/spc

%.info: $(modinfo) %.spc
	$^ > $@

%.dump: $(dumper) %.spc
	$^ > $@

