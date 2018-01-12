library_name := ht
path_step := ../..

source_files += $(addprefix Core/,\
  arm \
  dcsound \
  satsound \
  sega \
  yam \
  m68k/m68kcpu \
  m68k/m68kops \
)

defines += EMU_COMPILE EMU_LITTLE_ENDIAN LSB_FIRST USE_M68K NO_DYNAREC

source_files := $(addsuffix .c,$(source_files))

include $(path_step)/makefile.mak
