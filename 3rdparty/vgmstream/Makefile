library_name := vgmstream
dirs.root := ../..
include $(dirs.root)/make/default.mak

includes.dirs += $(dirs.root)/3rdparty $(dirs.root)/3rdparty/atrac9/C/src $(dirs.root)/3rdparty/mpg123/src/libmpg123 ext_includes ext_includes/ffmpeg

source_dirs = src src/base src/coding src/layout src/meta src/util

includes.files = config.h ext_includes/ffmpeg/libavutil/mathematics.h

include $(dirs.root)/makefile.mak
