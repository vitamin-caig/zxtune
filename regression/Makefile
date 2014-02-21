library_name = fake
zxtune123 = $(bins_dir)/$(call makebin_name,zxtune123)
path_step = ..
include $(path_step)/make/default.mak

all: $(zxtune123)
	$(zxtune123) --convert mode=debugay,filename=[Filename].debugay --providers-options file.overwrite=1 $(wildcard *)

$(zxtune123):
	$(MAKE) -C $(path_step)/apps/zxtune123 $(MAKECMDGOALS)

include $(path_step)/makefile.mak
