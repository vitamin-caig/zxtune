library_name := sidplayfp
path_step := ../..
include defines.mak

source_dirs = sidplayfp \
              sidplayfp/c64 \
              sidplayfp/c64/CIA \
              sidplayfp/c64/CPU \
              sidplayfp/c64/VIC_II \
              sidplayfp/sidtune \

source_dirs += utils/MD5

defines += FAST_AND_ROUGH NO_ZERORAM

include $(path_step)/makefile.mak
