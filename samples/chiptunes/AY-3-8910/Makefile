library_name = fake
zxtune123 = $(bins_dir)/$(call makebin_name,zxtune123)
dirs.root = ../../..
include $(dirs.root)/make/default.mak

all: $(zxtune123)
	find . -type f | xargs $(zxtune123) --convert mode=debugay,filename=regression/[Filename][Subpath].debugay --providers-options file.overwrite=1

$(zxtune123):
	$(MAKE) -C $(dirs.root)/apps/zxtune123 $(MAKECMDGOALS)

include $(dirs.root)/makefile.mak
