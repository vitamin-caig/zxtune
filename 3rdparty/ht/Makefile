library_name := ht
dirs.root := ../..

source_files += $(addprefix Core/,\
  arm \
  dcsound \
  satsound \
  sega \
  yam \
  c68k/c68k \
  c68k/c68kexec \
)

defines += EMU_COMPILE EMU_LITTLE_ENDIAN LSB_FIRST C68K_NO_JUMP_TABLE NO_DYNAREC HAVE_STDINT_H

source_files := $(addsuffix .c,$(source_files))

include $(dirs.root)/makefile.mak
