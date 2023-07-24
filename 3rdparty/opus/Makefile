library_name := opus
dirs.root := ../..
include $(dirs.root)/make/default.mak

includes.dirs += $(addprefix $(dirs.root)/3rdparty/opus/,include celt silk silk/float)

#TODO: use FIXED_POINT

include celt_sources.mk
include opus_sources.mk
include silk_sources.mk

source_files := $(CELT_SOURCES) $(OPUS_SOURCES) $(SILK_SOURCES) $(SILK_SOURCES_FLOAT)

includes.files = config.h

include $(dirs.root)/makefile.mak
