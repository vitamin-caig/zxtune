library_name := lzma
path_step := ../..
include $(path_step)/make/default.mak

source_files := $(addprefix C/,\
  7zArcIn.c \
  7zBuf.c 7zBuf2.c \
  7zCrc.c 7zCrcOpt.c \
  7zDec.c \
  7zStream.c \
  Bcj2.c Bra.c Bra86.c BraIA64.c \
  CpuArch.c \
  Delta.c \
  LzmaDec.c Lzma2Dec.c \
  Ppmd7.c Ppmd7Dec.c \
  )

defines += _7ZIP_PPMD_SUPPPORT  

include $(path_step)/makefile.mak
