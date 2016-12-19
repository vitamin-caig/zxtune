library_name := he
path_step := ../..

source_files += $(addprefix Core/,\
  bios \
  iop \
  ioptimer \
  psx \
  r3000 \
  spu \
  spucore \
  vfs \
)

defines += EMU_COMPILE EMU_LITTLE_ENDIAN

source_files := $(addsuffix .c,$(source_files))

include $(path_step)/makefile.mak
