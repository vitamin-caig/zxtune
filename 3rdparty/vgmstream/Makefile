library_name := vgmstream
dirs.root := ../..
include $(dirs.root)/make/default.mak

dirs.ext = $(dirs.root)/3rdparty

includes.dirs += $(dirs.ext)/atrac9/C/src $(dirs.ext)/mpg123/src/libmpg123 $(dirs.ext)/ffmpeg $(dirs.ext)

source_dirs = src src/base src/coding src/coding/libs src/layout src/meta src/util

includes.files = config.h 

include $(dirs.root)/makefile.mak
