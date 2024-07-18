library_name := sidplayfp
dirs.root := ../..

includes.dirs += $(dirs.root)/3rdparty/sidplayfp/src

source_dirs = src \
              src/c64 \
              src/c64/CIA \
              src/c64/CPU \
              src/c64/VIC_II \
              src/sidplayfp \
              src/sidtune \

source_dirs += src/utils/MD5
source_files += src/utils/md5Factory.cpp

source_dirs += src/builders/resid-builder

source_files += $(foreach src,dac envelope extfilt filter8580new pot sid version voice wave,src/builders/resid-builder/resid/$(src).cc)

includes.files += defines.h

include $(dirs.root)/makefile.mak

#-fdata-sections makes static class members be located in .data sections, not in .bss
mingw.cxx.flags += -fno-data-sections
