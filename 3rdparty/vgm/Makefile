library_name := vgm
dirs.root := ../..
include $(dirs.root)/make/default.mak

source_files = $(addprefix emu/cores/,\
  sn764intf sn76496 sn76489 \
  2413intf ym2413 nukedopll emu2413 \
  2612intf ym2612 ym3438 \
  ym2151 2151intf nukedopm \
  segapcm \
  rf5cintf rf5c68 scd_pcm \
  opnintf fmopn ymdeltat \
  oplintf adlibemu_opl2 fmopl \
  262intf nukedopl3 adlibemu_opl3 ymf262 \
  ymf278b \
  ymz280b \
  ymf271 \
  ayintf emu2149 ay8910 \
  pwm \
  gb \
  nesintf np_nes_apu np_nes_dmc np_nes_fds nes_apu \
  multipcm \
  upd7759 \
  okim6258 \
  okim6295 okiadpcm \
  k051649 \
  k054539 \
  c6280intf Ootake_PSG c6280_mame \
  c140 \
  c219 \
  k053260 \
  pokey \
  qsoundintf qsound_ctr qsound_mame \
  scsp scspdsp \
  ws_audio \
  vsu \
  saaintf saa1099_vb saa1099_mame \
  es5503 \
  es5506 \
  x1_010 \
  c352 \
  iremga20 \
)

source_files += $(addprefix emu/,SoundEmu Resampler panning dac_control logging_stub)

source_files += $(addprefix utils/,DataLoader StrUtils-CPConv_Stub)

source_files := $(addsuffix .c,$(source_files))

source_files += $(addprefix player/,playerbase.cpp s98player.cpp vgmplayer.cpp vgmplayer_cmdhandler.cpp helper.c dblk_compr.c)

defines += HAVE_STDINT_H

include $(dirs.root)/makefile.mak
