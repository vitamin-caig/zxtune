library_name := sidplayfp
path_step := ../..
include defines.mak

includes.dirs += $(path_step)/3rdparty/sidplayfp/sidplayfp

source_dirs = sidplayfp \
              sidplayfp/c64 \
              sidplayfp/c64/CIA \
              sidplayfp/c64/CPU \
              sidplayfp/c64/VIC_II \
              sidplayfp/sidtune \

source_dirs += utils/MD5

source_dirs += builders/resid-builder \
               builders/resid-builder/resid

#disable for digital tunes playback (e.g. hvsc/MUSICIANS/L/LMan/Vortex.sid)
#defines += FAST_AND_ROUGH

includes.files += defines.h

include $(path_step)/makefile.mak

#-fdata-sections makes static class members be located in .data sections, not in .bss
mingw.cxx.flags += -fno-data-sections
