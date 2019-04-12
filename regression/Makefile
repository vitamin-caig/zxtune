library_name = fake
zxtune123 = $(bins_dir)/$(call makebin_name,zxtune123)
dirs.root = ..
include $(dirs.root)/make/default.mak

all: $(zxtune123)
	$(zxtune123) --convert mode=debugay,filename=[Filename][Subpath].debugay --providers-options file.overwrite=1 $(wildcard *)

$(zxtune123):
	$(MAKE) -C $(dirs.root)/apps/zxtune123 $(MAKECMDGOALS)

include $(dirs.root)/makefile.mak
