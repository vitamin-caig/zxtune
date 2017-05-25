library_name := sidplayfp
path_step := ../..
include defines.mak

include_dirs += $(path_step)/3rdparty/sidplayfp/sidplayfp

source_dirs = sidplayfp \
              sidplayfp/c64 \
              sidplayfp/c64/CIA \
              sidplayfp/c64/CPU \
              sidplayfp/c64/VIC_II \
              sidplayfp/sidtune \

source_dirs += utils/MD5

source_dirs += builders/resid-builder \
               builders/resid-builder/resid
               
defines += FAST_AND_ROUGH

include_files = defines.h

include $(path_step)/makefile.mak

#-fdata-sections makes static class members be located in .data sections, not in .bss
mingw.cxx.flags += -fno-data-sections
