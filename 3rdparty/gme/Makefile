library_name := gme
path_step := ../..
include $(path_step)/make/default.mak

include_dirs = .
#common
source_files = $(addprefix gme/,\
                 blargg_common.cpp blargg_errors.cpp \
                 Data_Reader.cpp Gme_File.cpp Gme_Loader.cpp Music_Emu.cpp Classic_Emu.cpp Blip_Buffer.cpp Multi_Buffer.cpp Track_Filter.cpp Rom_Data.cpp)

#nsf
source_files += $(addprefix gme/,Nsf_Emu.cpp Nsf_Core.cpp Nsf_Cpu.cpp Nsf_Impl.cpp Nsfe_Emu.cpp)
source_files += $(addprefix gme/,Nes_Apu.cpp Nes_Cpu.cpp Nes_Oscs.cpp)
#$(platform)_definitions += NSF_EMU_APU_ONLY
source_files += $(addprefix gme/,Nes_Fds_Apu.cpp Nes_Fme7_Apu.cpp Nes_Namco_Apu.cpp Nes_Vrc6_Apu.cpp Nes_Vrc7_Apu.cpp ym2413.c)

#gbs
source_files += $(addprefix gme/,Gbs_Emu.cpp Gbs_Core.cpp Gbs_Cpu.cpp Gb_Apu.cpp Gb_Cpu.cpp Gb_Oscs.cpp)

#sap
source_files += $(addprefix gme/,Sap_Emu.cpp Sap_Core.cpp Sap_Apu.cpp Sap_Cpu.cpp)

#hes
source_files += $(addprefix gme/,Hes_Emu.cpp Hes_Core.cpp Hes_Apu.cpp Hes_Apu_Adpcm.cpp Hes_Cpu.cpp)

include $(path_step)/makefile.mak
