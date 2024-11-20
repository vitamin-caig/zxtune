ifdef build_dll
dynamic_name := FLAC
libraries.windows := oldnames
static_runtime = 1
defines += FLAC_API_EXPORTS
else
library_name := FLAC
defines += FLAC__NO_DLL
endif

dirs.root := ../..
include $(dirs.root)/make/default.mak 

defines += FLAC__NO_ASM PACKAGE_VERSION=\"1.3.3\" FLAC__HAS_OGG=0 HAVE_LROUND HAVE_STDINT_H
defines.mingw = HAVE_FSEEKO
defines.linux = HAVE_FSEEKO

includes.dirs = src/libFLAC/include . ..
sources = bitmath bitreader bitwriter cpu crc fixed float format lpc md5 memory metadata_iterators metadata_object stream_decoder stream_encoder stream_encoder_framing \
 window

source_files = $(addprefix src/libFLAC/,$(addsuffix .c,$(sources)))
source_files.mingw = src/share/win_utf8_io/win_utf8_io.c src/libFLAC/windows_unicode_filenames.c
source_files.windows = $(source_files.mingw)

include $(dirs.root)/makefile.mak
