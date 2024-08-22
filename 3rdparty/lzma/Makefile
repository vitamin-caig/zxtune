library_name := lzma
dirs.root := ../..
include $(dirs.root)/make/default.mak

source_files := $(addprefix C/,\
  7zArcIn.c \
  7zBuf.c 7zBuf2.c \
  7zCrc.c 7zCrcOpt.c \
  7zDec.c \
  7zStream.c \
  Bcj2.c Bra.c Bra86.c \
  Delta.c \
  LzmaDec.c Lzma2Dec.c \
  Ppmd7.c Ppmd7Dec.c \
  )

defines += Z7_PPMD_SUPPORT Z7_EXTRACT_ONLY

include $(dirs.root)/makefile.mak
