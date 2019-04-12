library_name := he
dirs.root := ../..

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

include $(dirs.root)/makefile.mak
