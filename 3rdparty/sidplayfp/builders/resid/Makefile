library_name := sidplayfp_builders_resid
path_step := ../../../..

source_dirs = . resid

include ../../defines.mak
include $(path_step)/makefile.mak

#-fdata-sections makes static class members be located in .data sections, not in .bss
mingw.cxx.flags += -fno-data-sections
