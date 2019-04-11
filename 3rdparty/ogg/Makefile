#	https://git.xiph.org/mirrors/ogg.git

dynamic_name := ogg
path_step := ../..
include $(path_step)/make/default.mak 

defines += LIBOGG_EXPORTS

includes.dirs += include
source_files_cc := src/bitwise src/framing
libraries.3rdparty := zlib

LD_PLATFORM_FLAGS = /DEF:win32\ogg.def

static_runtime = 1

include $(path_step)/makefile.mak
