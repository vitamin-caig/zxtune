# v1.3.7 from https://gitlab.xiph.org/xiph/vorbis.git
library_name := vorbis
dirs.root := ../..
include $(dirs.root)/make/default.mak

includes.dirs += $(dirs.root)/3rdparty

source_files = $(addprefix lib/,bitrate block codebook envelope floor0 floor1 info lpc lsp mapping0 mdct psy registry res0 sharedbook smallft synthesis vorbisfile window)

source_files := $(addsuffix .c,$(source_files))

defines += alloca=malloc

include $(dirs.root)/makefile.mak
