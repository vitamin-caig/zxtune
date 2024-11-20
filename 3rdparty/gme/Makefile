library_name := gme
dirs.root := ../..
include $(dirs.root)/make/default.mak

includes.dirs += .
#common
source_files = $(addprefix gme/,\
                 Data_Reader.cpp Gme_File.cpp Music_Emu.cpp Classic_Emu.cpp Blip_Buffer.cpp Multi_Buffer.cpp)

#nsf
source_files += $(addprefix gme/,Nsf_Emu.cpp Nsfe_Emu.cpp)
source_files += $(addprefix gme/,Nes_Apu.cpp Nes_Cpu.cpp Nes_Oscs.cpp)
#defines += NSF_EMU_APU_ONLY
source_files += $(addprefix gme/,Nes_Fds_Apu.cpp Nes_Fme7_Apu.cpp Nes_Namco_Apu.cpp Nes_Vrc6_Apu.cpp Nes_Vrc7_Apu.cpp) \
                $(addprefix gme/ext/,emu2413.c panning.c)

#gbs
source_files += $(addprefix gme/,Gbs_Emu.cpp Gb_Cpu.cpp Gb_Apu.cpp Gb_Cpu.cpp Gb_Oscs.cpp)

#hes
source_files += $(addprefix gme/,Hes_Emu.cpp Hes_Apu.cpp Hes_Cpu.cpp)

#gym
source_files += $(addprefix gme/,Gym_Emu.cpp \
                 Ym2612_Nuked.cpp \
                 Sms_Apu.cpp \
                 )

#kss
source_files += $(addprefix gme/,Kss_Emu.cpp Kss_Cpu.cpp Kss_Scc_Apu.cpp \
                  Ay_Apu.cpp \
                 )

#generic
source_files += $(addprefix gme/,Fir_Resampler.cpp Dual_Resampler.cpp \
                 )

defines += HAVE_STDINT_H BLARGG_LITTLE_ENDIAN VGM_YM2612_NUKED

include $(dirs.root)/makefile.mak
