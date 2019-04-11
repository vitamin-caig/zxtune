#put file to src\libFLAC of https://git.xiph.org/flac.git

dynamic_name := FLAC
path_step := ../../../../..
include $(path_step)/make/default.mak 

defines += FLAC__NO_ASM FLAC_API_EXPORTS VERSION=\"1.3.0\"

include_dirs = include ../../include
source_files_cc := bitmath bitreader bitwriter cpu crc fixed float format lpc md5 memory metadata_iterators metadata_object stream_decoder stream_encoder stream_encoder_framing \
 window ../share/win_utf8_io/win_utf8_io
libraries := oldnames

static_runtime = 1

include $(path_step)/makefile.mak
