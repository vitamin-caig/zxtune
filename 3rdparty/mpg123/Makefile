library_name := mpg123
dirs.root := ../..
include $(dirs.root)/make/default.mak

includes.dirs = src src/compat src/libmpg123

compat_files = compat_str
lib_files = libmpg123 frame icy icy2utf8 id3 index dct64 dither format layer1 layer2 layer3 ntom optimize parse readers stringbuf synth synth_8bit synth_real synth_s32 tabinit

#TODO: support different hardware platforms
lib_files += getcpuflags_stub
defines += NO_FILEIO REAL_IS_FLOAT OPT_GENERIC OPT_GENERIC_DITHER

source_files := $(addsuffix .c,$(addprefix src/libmpg123/,$(lib_files)) $(addprefix src/compat/,$(compat_files)))

include $(dirs.root)/makefile.mak
