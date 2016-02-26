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

#vgm
source_files += $(addprefix gme/,Vgm_Emu.cpp Vgm_Core.cpp Fir_Resampler.cpp Dual_Resampler.cpp Resampler.cpp \
                  Ay_Apu.cpp \
                  C140_Emu.cpp c140.c\
                  dac_control.c \
                  K051649_Emu.cpp k051649.c \
                  K053260_Emu.cpp k053260.c \
                  K054539_Emu.cpp k054539.c \
                  Okim6258_Emu.cpp okim6258.c \
                  Okim6295_Emu.cpp okim6295.c \
                  Pwm_Emu.cpp pwm.c \
                  Qsound_Apu.cpp qmix.c \
                  Rf5C164_Emu.cpp scd_pcm.c \
                  Rf5C68_Emu.cpp rf5c68.c \
                  SegaPcm_Emu.cpp segapcm.c \
                  Sms_Apu.cpp \
                  Ym2151_Emu.cpp ym2151.c \
                  Ym2203_Emu.cpp fm.c ymdeltat.cpp \
                  Ym2413_Emu.cpp \
                  Ym2608_Emu.cpp \
                  Ym2610b_Emu.cpp \
                  Ym2612_Emu.cpp fm2612.c \
                  Ym3812_Emu.cpp \
                  Ymf262_Emu.cpp dbopl.cpp \
                  Ymz280b_Emu.cpp ymz280b.c \
                 )
                 
#gym
source_files += $(addprefix gme/,Gym_Emu.cpp)
#                 Ym2612_Emu.cpp fm2612.c \
#                 Sms_Apu.cpp \

#kss
source_files += $(addprefix gme/,Kss_Emu.cpp Kss_Core.cpp Kss_Cpu.cpp Kss_Scc_Apu.cpp \
                  Z80_Cpu.cpp Opl_Apu.cpp Ym3812_Emu.cpp fmopl.cpp \
                 )
 
include $(path_step)/makefile.mak
