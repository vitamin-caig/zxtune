makebin_name = $(1)
makelib_name = lib$(1).a
makedyn_name = lib$(1).dylib
makeobj_name = $(1).o
makedir_cmd = mkdir -p $(1)
rmdir_cmd = rm -Rf $(1)
rmfiles_cmd = rm -f $(1)
showtime_cmd = date +"%x %X"

compiler=clang
host=macos
LINKER_BEGIN_GROUP=
LINKER_END_GROUP=
postlink_cmd = $(tools.strip) $@ && touch $@.pdb

darwin.ld.flags += $(foreach file,$(embedded_files),-sectcreate __TEXT __emb_$(basename $(notdir $(file))) $(file))
ifndef profile
ifdef release
darwin.ld.flags += -Wl,-dead_strip
endif
endif
