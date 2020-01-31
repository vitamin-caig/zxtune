library_name := ffmpeg
dirs.root := ../..

source_files = $(addprefix libavcodec/, \
               allcodecs avpacket bitstream bitstream_filters null_bsf decode dct dct32_fixed dct32_float fft_float mdct_float options rdft utils)\
               $(addprefix libavutil/, avstring bprint bsf buffer channel_layout crc dict error eval float_dsp frame lfg log2_tab \
               mathematics mem md5 parseutils rational reverse samplefmt sha sinewin time opt \
               imgutils hwcontext pixdesc)\
               $(addprefix stubs/, codec_desc_stub log_stub random_seed_stub util_stub)

includes.dirs = . compat/atomics/dummy

#atrac3,atrac3+
source_files += $(addprefix libavcodec/, \
               atrac atrac3 atrac3plus atrac3plusdec atrac3plusdsp)

#atrac9
source_files += $(addprefix libavcodec/, \
                atrac9dec)

source_files := $(addsuffix .c,$(source_files))

defines += av_restrict=restrict HAVE_AV_CONFIG_H

include $(dirs.root)/makefile.mak
