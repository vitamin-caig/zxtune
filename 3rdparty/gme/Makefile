library_name := gme
dirs.root := ../..
include $(dirs.root)/make/default.mak

includes.dirs += .
#common
source_files = $(addprefix gme/,\
                 blargg_common.cpp blargg_errors.cpp \
                 Data_Reader.cpp Gme_File.cpp Gme_Loader.cpp Music_Emu.cpp Classic_Emu.cpp Blip_Buffer.cpp Multi_Buffer.cpp Track_Filter.cpp Rom_Data.cpp)

#nsf
source_files += $(addprefix gme/,Nsf_Emu.cpp Nsf_Core.cpp Nsf_Cpu.cpp Nsf_Impl.cpp Nsfe_Emu.cpp)
source_files += $(addprefix gme/,Nes_Apu.cpp Nes_Cpu.cpp Nes_Oscs.cpp)
#defines += NSF_EMU_APU_ONLY
source_files += $(addprefix gme/,Nes_Fds_Apu.cpp Nes_Fme7_Apu.cpp Nes_Namco_Apu.cpp Nes_Vrc6_Apu.cpp Nes_Vrc7_Apu.cpp ym2413.cpp)

#gbs
source_files += $(addprefix gme/,Gbs_Emu.cpp Gbs_Core.cpp Gbs_Cpu.cpp Gb_Apu.cpp Gb_Cpu.cpp Gb_Oscs.cpp)

#hes
source_files += $(addprefix gme/,Hes_Emu.cpp Hes_Core.cpp Hes_Apu.cpp Hes_Apu_Adpcm.cpp Hes_Cpu.cpp)

#gym
source_files += $(addprefix gme/,Gym_Emu.cpp \
                 Ym2612_Emu.cpp fm2612.cpp \
                 Sms_Apu.cpp \
                 )

#kss
source_files += $(addprefix gme/,Kss_Emu.cpp Kss_Core.cpp Kss_Cpu.cpp Kss_Scc_Apu.cpp \
                  Z80_Cpu.cpp Opl_Apu.cpp fmopl.cpp \
                  Ay_Apu.cpp \
                 )

#generic
source_files += $(addprefix gme/,Resampler.cpp Fir_Resampler.cpp Dual_Resampler.cpp \
                 )

defines += HAVE_STDINT_H

include $(dirs.root)/makefile.mak
