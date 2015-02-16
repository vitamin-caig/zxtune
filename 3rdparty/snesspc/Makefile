library_name := snesspc
path_step := ../..
include $(path_step)/make/default.mak

$(platform)_definitions += BLARGG_COMPILER_HAS_BOOL SPC_NO_COPY_STATE_FUNCS
#SPC_LESS_ACCURATE SPC_DISABLE_TEMPO

include_dirs = .
source_files := $(addprefix snes_spc/,SNES_SPC.cpp SNES_SPC_misc.cpp SPC_DSP.cpp SPC_Filter.cpp)

include $(path_step)/makefile.mak
