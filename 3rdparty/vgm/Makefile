library_name := vgm
dirs.root := ../..
include $(dirs.root)/make/default.mak

define device
core_defines += $(1)
core_files += $(2)
endef

#mame default
$(eval $(call device,SNDDEV_SN76496 EC_SN76496_MAME, sn764intf sn76496))
#mame default
$(eval $(call device,SNDDEV_YM2413 EC_YM2413_MAME, 2413intf ym2413))
#gpgx default
$(eval $(call device,SNDDEV_YM2612 EC_YM2612_GPGX, 2612intf fmopn2612))
$(eval $(call device,SNDDEV_YM2151, ym2151))
$(eval $(call device,SNDDEV_SEGAPCM, segapcm))
#both cores used
$(eval $(call device,SNDDEV_RF5C68 EC_RF5C68_MAME SNDEMU_RF5C68_GENS, rf5cintf rf5c68 scd_pcm))
$(eval $(call device,SNDDEV_YM2203, opnintf fmopn))
$(eval $(call device,SNDDEV_YM2608 SNDDEV_YM2610, opnintf fmopn ymdeltat))
#adlibemu default
$(eval $(call device,SNDDEV_YM3812 EC_YM3812_ADLIBEMU, oplintf adlibemu_opl2 fmopl))
$(eval $(call device,SNDDEV_YM3526, oplintf fmopl))
$(eval $(call device,SNDDEV_Y8950, fmopl ymdeltat))
#nuke default, adlib for 278b
$(eval $(call device,SNDDEV_YMF262 EC_YMF262_NUKED EC_YMF262_ADLIBEMU, 262intf nukedopl adlibemu_opl3))
$(eval $(call device,SNDDEV_YMF278B, ymf278b))
$(eval $(call device,SNDDEV_YMZ280B, ymz280b))
$(eval $(call device,SNDDEV_YMF271, ymf271))
#fcc_emu default
$(eval $(call device,SNDDEV_AY8910 EC_AY8910_EMU2149, ayintf emu2149))
$(eval $(call device,SNDDEV_32X_PWM, pwm))
$(eval $(call device,SNDDEV_GAMEBOY, gb))
#nsfp default with fds
$(eval $(call device,SNDDEV_NES_APU EC_NES_NSFPLAY EC_NES_NSFP_FDS, nesintf np_nes_apu np_nes_dmc np_nes_fds))
$(eval $(call device,SNDDEV_YMW258, multipcm))
$(eval $(call device,SNDDEV_UPD7759, upd7759))
$(eval $(call device,SNDDEV_OKIM6258, okim6258))
$(eval $(call device,SNDDEV_OKIM6295, okim6295 okiadpcm))
$(eval $(call device,SNDDEV_K051649, k051649))
$(eval $(call device,SNDDEV_K054539, k054539))
#ootk default
$(eval $(call device,SNDDEV_C6280 EC_C6280_OOTAKE, c6280intf Ootake_PSG))
$(eval $(call device,SNDDEV_C140, c140))
$(eval $(call device,SNDDEV_C219, c219))
$(eval $(call device,SNDDEV_K053260, k053260))
$(eval $(call device,SNDDEV_POKEY, pokey))
#ctr default
$(eval $(call device,SNDDEV_QSOUND EC_QSOUND_CTR, qsoundintf qsound_ctr))
$(eval $(call device,SNDDEV_SCSP, scsp scspdsp))
$(eval $(call device,SNDDEV_WSWAN, ws_audio))
$(eval $(call device,SNDDEV_VBOY_VSU, vsu))
#vbel default
$(eval $(call device,SNDDEV_SAA1099 EC_SAA1099_VB, saaintf saa1099_vb))
$(eval $(call device,SNDDEV_ES5503, es5503))
$(eval $(call device,SNDDEV_ES5506, es5506))
$(eval $(call device,SNDDEV_X1_010, x1_010))
$(eval $(call device,SNDDEV_C352, c352))
$(eval $(call device,SNDDEV_GA20, iremga20))

defines += NO_EXTERNAL_ROM SNDDEV_SELECT $(sort $(core_defines))
source_files = $(addprefix emu/,SoundEmu Resampler panning dac_control) \
               $(addprefix emu/cores/,$(sort $(core_files)))

source_files += $(addprefix utils/,DataLoader StrUtils-CPConv_Stub)

source_files := $(addsuffix .c,$(source_files))

source_files += $(addprefix player/,playerbase.cpp s98player.cpp vgmplayer.cpp vgmplayer_cmdhandler.cpp helper.c dblk_compr.c)

include $(dirs.root)/makefile.mak
