library_name := lhasa
path_step := ../..
include $(path_step)/make/default.mak

source_files := $(addprefix lib/,\
	crc16 \
	ext_header \
	lha_arch_unix \
	lha_arch_win32 \
	lha_decoder \
	lha_endian \
	lha_file_header \
	lha_input_stream \
	lha_basic_reader \
	lha_reader \
	macbinary \
	null_decoder \
	lh1_decoder \
	lh5_decoder \
	lh6_decoder \
	lh7_decoder \
	lz5_decoder \
	lzs_decoder \
	pm2_decoder)

#this files are included directly:	bit_stream_reader lh_new_decoder tree_decode

defines.windows += vsnprintf=_vsnprintf

source_files := $(addsuffix .c,$(source_files))

include $(path_step)/makefile.mak
