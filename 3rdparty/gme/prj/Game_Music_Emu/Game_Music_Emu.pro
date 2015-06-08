#-------------------------------------------------
#
# Project created by QtCreator 2012-12-20T17:10:09
#
#-------------------------------------------------

QT       -= core gui

TARGET = Game_Music_Emu
TEMPLATE = lib
CONFIG += staticlib c++11

QMAKE_CXXFLAGS += -std=c++11
macx:QMAKE_CXXFLAGS += -mmacosx-version-min=10.7 -stdlib=libc++

DEFINES += NDEBUG HAVE_STDINT_H HAVE_ZLIB_H

INCLUDEPATH += ../../../File_Extractor/fex

SOURCES += \
    ../../gme/Z80_Cpu.cpp \
    ../../gme/ymz280b.c \
    ../../gme/Ymz280b_Emu.cpp \
    ../../gme/Ymf262_Emu.cpp \
    ../../gme/ymdeltat.cpp \
    ../../gme/Ym3812_Emu.cpp \
    ../../gme/Ym2612_Emu.cpp \
    ../../gme/Ym2612_Emu_MAME.cpp \
    ../../gme/Ym2612_Emu_Gens.cpp \
    ../../gme/Ym2610b_Emu.cpp \
    ../../gme/Ym2608_Emu.cpp \
    ../../gme/ym2413.c \
    ../../gme/Ym2413_Emu.cpp \
    ../../gme/Ym2203_Emu.cpp \
    ../../gme/ym2151.c \
    ../../gme/Ym2151_Emu.cpp \
    ../../gme/Vgm_Emu.cpp \
    ../../gme/Vgm_Core.cpp \
    ../../gme/Upsampler.cpp \
    ../../gme/Track_Filter.cpp \
    ../../gme/Spc_Filter.cpp \
    ../../gme/Spc_Emu.cpp \
    ../../gme/Sms_Fm_Apu.cpp \
    ../../gme/Sms_Apu.cpp \
    ../../gme/Sgc_Impl.cpp \
    ../../gme/Sgc_Emu.cpp \
    ../../gme/Sgc_Cpu.cpp \
    ../../gme/Sgc_Core.cpp \
    ../../gme/segapcm.c \
    ../../gme/SegaPcm_Emu.cpp \
    ../../gme/scd_pcm.c \
    ../../gme/Sap_Emu.cpp \
    ../../gme/Sap_Cpu.cpp \
    ../../gme/Sap_Core.cpp \
    ../../gme/Sap_Apu.cpp \
    ../../gme/s_opltbl.c \
    ../../gme/s_opl.c \
    ../../gme/s_logtbl.c \
    ../../gme/s_deltat.c \
    ../../gme/Rom_Data.cpp \
    ../../gme/Rf5C164_Emu.cpp \
    ../../gme/rf5c68.c \
    ../../gme/Rf5C68_Emu.cpp \
    ../../gme/Resampler.cpp \
    ../../gme/pwm.c \
    ../../gme/Pwm_Emu.cpp \
    ../../gme/Opl_Apu.cpp \
    ../../gme/okim6295.c \
    ../../gme/Okim6295_Emu.cpp \
    ../../gme/okim6258.c \
    ../../gme/Okim6258_Emu.cpp \
    ../../gme/Nsfe_Emu.cpp \
    ../../gme/Nsf_Impl.cpp \
    ../../gme/Nsf_Emu.cpp \
    ../../gme/Nsf_Cpu.cpp \
    ../../gme/Nsf_Core.cpp \
    ../../gme/Nes_Vrc7_Apu.cpp \
    ../../gme/Nes_Vrc6_Apu.cpp \
    ../../gme/Nes_Oscs.cpp \
    ../../gme/Nes_Namco_Apu.cpp \
    ../../gme/Nes_Fme7_Apu.cpp \
    ../../gme/Nes_Fds_Apu.cpp \
    ../../gme/Nes_Cpu.cpp \
    ../../gme/Nes_Apu.cpp \
    ../../gme/Music_Emu.cpp \
    ../../gme/Multi_Buffer.cpp \
    ../../gme/M3u_Playlist.cpp \
    ../../gme/Kss_Scc_Apu.cpp \
    ../../gme/Kss_Emu.cpp \
    ../../gme/Kss_Cpu.cpp \
    ../../gme/Kss_Core.cpp \
    ../../gme/k054539.c \
    ../../gme/K054539_Emu.cpp \
    ../../gme/k053260.c \
    ../../gme/K053260_Emu.cpp \
    ../../gme/k051649.c \
    ../../gme/K051649_Emu.cpp \
    ../../gme/Hes_Emu.cpp \
    ../../gme/Hes_Cpu.cpp \
    ../../gme/Hes_Core.cpp \
    ../../gme/Hes_Apu.cpp \
    ../../gme/Hes_Apu_Adpcm.cpp \
    ../../gme/Gym_Emu.cpp \
    ../../gme/gme.cpp \
    ../../gme/Gme_Loader.cpp \
    ../../gme/Gme_File.cpp \
    ../../gme/Gbs_Emu.cpp \
    ../../gme/Gbs_Cpu.cpp \
    ../../gme/Gbs_Core.cpp \
    ../../gme/Gb_Oscs.cpp \
    ../../gme/Gb_Cpu.cpp \
    ../../gme/Gb_Apu.cpp \
    ../../gme/fmopl.cpp \
    ../../gme/fm2612.c \
    ../../gme/fm.c \
    ../../gme/Fir_Resampler.cpp \
    ../../gme/Effects_Buffer.cpp \
    ../../gme/Dual_Resampler.cpp \
    ../../gme/Downsampler.cpp \
    ../../gme/dbopl.cpp \
    ../../gme/dac_control.c \
    ../../gme/Classic_Emu.cpp \
    ../../gme/c140.c \
    ../../gme/C140_Emu.cpp \
    ../../gme/Blip_Buffer.cpp \
    ../../gme/blargg_errors.cpp \
    ../../gme/blargg_common.cpp \
    ../../gme/Ay_Emu.cpp \
    ../../gme/Ay_Cpu.cpp \
    ../../gme/Ay_Core.cpp \
    ../../gme/Ay_Apu.cpp \
    ../../gme/qmix.c \
    ../../gme/Qsound_Apu.cpp \
    ../../gme/Bml_Parser.cpp \
    ../../gme/Spc_Sfm.cpp \
    ../../gme/higan/processor/spc700/spc700.cpp \
    ../../gme/higan/dsp/SPC_DSP.cpp \
    ../../gme/higan/dsp/dsp.cpp \
    ../../gme/higan/smp/smp.cpp

HEADERS += \
    ../../gme/Z80_Cpu.h \
    ../../gme/Z80_Cpu_run.h \
    ../../gme/ymz280b.h \
    ../../gme/Ymz280b_Emu.h \
    ../../gme/Ymf262_Emu.h \
    ../../gme/ymdeltat.h \
    ../../gme/Ym3812_Emu.h \
    ../../gme/Ym2612_Emu.h \
    ../../gme/Ym2610b_Emu.h \
    ../../gme/Ym2608_Emu.h \
    ../../gme/ym2413.h \
    ../../gme/Ym2413_Emu.h \
    ../../gme/Ym2203_Emu.h \
    ../../gme/ym2151.h \
    ../../gme/Ym2151_Emu.h \
    ../../gme/Vgm_Emu.h \
    ../../gme/Vgm_Core.h \
    ../../gme/Upsampler.h \
    ../../gme/Track_Filter.h \
    ../../gme/Spc_Filter.h \
    ../../gme/Spc_Emu.h \
    ../../gme/Sms_Fm_Apu.h \
    ../../gme/Sms_Apu.h \
    ../../gme/Sgc_Impl.h \
    ../../gme/Sgc_Emu.h \
    ../../gme/Sgc_Core.h \
    ../../gme/segapcm.h \
    ../../gme/SegaPcm_Emu.h \
    ../../gme/scd_pcm.h \
    ../../gme/Sap_Emu.h \
    ../../gme/Sap_Core.h \
    ../../gme/Sap_Apu.h \
    ../../gme/s_opltbl.h \
    ../../gme/s_opl.h \
    ../../gme/s_logtbl.h \
    ../../gme/s_deltat.h \
    ../../gme/Rom_Data.h \
    ../../gme/Rf5C164_Emu.h \
    ../../gme/rf5c68.h \
    ../../gme/Rf5C68_Emu.h \
    ../../gme/Resampler.h \
    ../../gme/pwm.h \
    ../../gme/Pwm_Emu.h \
    ../../gme/Opl_Apu.h \
    ../../gme/okim6295.h \
    ../../gme/Okim6295_Emu.h \
    ../../gme/okim6258.h \
    ../../gme/Okim6258_Emu.h \
    ../../gme/Nsfe_Emu.h \
    ../../gme/Nsf_Impl.h \
    ../../gme/Nsf_Emu.h \
    ../../gme/Nsf_Core.h \
    ../../gme/nestypes.h \
    ../../gme/Nes_Vrc7_Apu.h \
    ../../gme/Nes_Vrc6_Apu.h \
    ../../gme/Nes_Oscs.h \
    ../../gme/Nes_Namco_Apu.h \
    ../../gme/Nes_Mmc5_Apu.h \
    ../../gme/Nes_Fme7_Apu.h \
    ../../gme/Nes_Fds_Apu.h \
    ../../gme/Nes_Cpu.h \
    ../../gme/Nes_Cpu_run.h \
    ../../gme/Nes_Apu.h \
    ../../gme/Music_Emu.h \
    ../../gme/Multi_Buffer.h \
    ../../gme/mamedef.h \
    ../../gme/M3u_Playlist.h \
    ../../gme/Kss_Scc_Apu.h \
    ../../gme/Kss_Emu.h \
    ../../gme/Kss_Core.h \
    ../../gme/kmsnddev.h \
    ../../gme/k054539.h \
    ../../gme/K054539_Emu.h \
    ../../gme/k053260.h \
    ../../gme/K053260_Emu.h \
    ../../gme/k051649.h \
    ../../gme/K051649_Emu.h \
    ../../gme/i_vrc7.h \
    ../../gme/i_fmunit.h \
    ../../gme/i_fmpac.h \
    ../../gme/Hes_Emu.h \
    ../../gme/Hes_Cpu.h \
    ../../gme/Hes_Cpu_run.h \
    ../../gme/Hes_Core.h \
    ../../gme/Hes_Apu.h \
    ../../gme/Hes_Apu_Adpcm.h \
    ../../gme/Gym_Emu.h \
    ../../gme/gme.h \
    ../../gme/Gme_Loader.h \
    ../../gme/Gme_File.h \
    ../../gme/Gbs_Emu.h \
    ../../gme/Gbs_Core.h \
    ../../gme/Gb_Oscs.h \
    ../../gme/Gb_Cpu.h \
    ../../gme/Gb_Cpu_run.h \
    ../../gme/Gb_Apu.h \
    ../../gme/fmopl.h \
    ../../gme/fm.h \
    ../../gme/Fir_Resampler.h \
    ../../gme/Effects_Buffer.h \
    ../../gme/Dual_Resampler.h \
    ../../gme/Downsampler.h \
    ../../gme/divfix.h \
    ../../gme/dbopl.h \
    ../../gme/dac_control.h \
    ../../gme/Classic_Emu.h \
    ../../gme/Chip_Resampler.h \
    ../../gme/c140.h \
    ../../gme/C140_Emu.h \
    ../../gme/Blip_Buffer.h \
    ../../gme/Blip_Buffer_impl2.h \
    ../../gme/Blip_Buffer_impl.h \
    ../../gme/blargg_source.h \
    ../../gme/blargg_errors.h \
    ../../gme/blargg_endian.h \
    ../../gme/blargg_config.h \
    ../../gme/blargg_common.h \
    ../../gme/Ay_Emu.h \
    ../../gme/Ay_Core.h \
    ../../gme/Ay_Apu.h \
    ../../gme/adlib.h \
    ../../gme/qmix.h \
    ../../gme/Qsound_Apu.h \
    ../../gme/Bml_Parser.h \
    ../../gme/Spc_Sfm.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
