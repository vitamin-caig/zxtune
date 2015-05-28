library_name := gme
path_step := ../..
include $(path_step)/make/default.mak

include_dirs = .
#common
source_files = $(addprefix gme/,Data_Reader.cpp Gme_File.cpp Music_Emu.cpp Classic_Emu.cpp Blip_Buffer.cpp Multi_Buffer.cpp)

#nsf
source_files += $(addprefix gme/,Nsf_Emu.cpp Nsfe_Emu.cpp)
source_files += $(addprefix gme/,Nes_Apu.cpp Nes_Cpu.cpp Nes_Oscs.cpp)
#$(platform)_definitions += NSF_EMU_APU_ONLY
source_files += $(addprefix gme/,Nes_Namco_Apu.cpp Nes_Vrc6_Apu.cpp Nes_Fme7_Apu.cpp)

include $(path_step)/makefile.mak
