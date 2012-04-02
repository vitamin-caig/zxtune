dynamic_name := FLAC
path_step := ../../../..
include $(path_step)/make/default.mak 

$(platform)_definitions += FLAC__NO_ASM FLAC_API_EXPORTS VERSION=\"1.2.1\"

include_dirs = include ../../include
source_files_cc := bitmath bitreader bitwriter cpu crc fixed float format lpc md5 memory metadata_iterators metadata_object stream_decoder stream_encoder stream_encoder_framing window
libraries := oldnames

include $(path_step)/makefile.mak
