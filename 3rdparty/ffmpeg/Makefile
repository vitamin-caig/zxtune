library_name := ffmpeg
dirs.root := ../..

#fft + avfft?
avcodec_sources = fft_float fft_fixed_32 fft_init_table
#mdct
avcodec_sources += mdct_float mdct_fixed_32 mdct15
#dct
avcodec_sources += dct dct32_fixed dct32_float
#bsf
avcodec_sources += bitstream bitstream_filters bsf null_bsf

avcodec_sources += allcodecs avcodec avpacket codec_par decode encode fmtconvert get_buffer mathtables mpegaudiotabs \
                mpeg4audio_sample_rates options parser parsers raw rdft sinewin to_upper4 utils vlc
avformat_sources += allformats avformat aviobuf avio demux demux_utils format id3v1 id3v2 metadata mux_utils options \
                 protocols replaygain seek utils
avutil_sources += aes aes_ctr audio_fifo avsscanf avstring base64 bprint buffer channel_layout crc des dict display downmix_info \
               encryption_info error eval fifo fixed_dsp float_dsp frame lfg log2_tab \
               mastering_display_metadata mathematics mem md5 parseutils rational rc4 reverse samplefmt sha time timecode opt \
               imgutils hwcontext pixdesc spherical stereo3d
source_files = $(addprefix stubs/, codec_desc_stub cpu_stub log_stub random_seed_stub util_stub)\
               $(addprefix libswresample/,audioconvert dither options rematrix resample resample_dsp swresample)

includes.dirs = . compat/atomics/dummy

#atrac3,atrac3+
avcodec_sources += atrac atrac3 atrac3plus atrac3plusdec atrac3plusdsp

#atrac9
avcodec_sources += atrac9dec

#mp3
avformat_sources += mp3dec
avcodec_sources += mpegaudiodecheader mpegaudio mpegaudiodata mpegaudiodec_common mpegaudiodec_fixed mpegaudiodsp mpegaudiodsp_data \
                mpegaudiodsp_fixed mpegaudiodsp_float mpc mpegaudio_parser

#bik
avformat_sources += bink
avcodec_sources += binkaudio wma_freqs

#aac, fixed
avformat_sources += aacdec apetag img2
avcodec_sources += aacdec_fixed aacps_common aacps_fixed aacpsdsp_fixed aacsbr_fixed aactab adts_header cbrt_data_fixed \
                sbrdsp_fixed kbdwin mpeg4audio

#mov
avformat_sources += mov mov_esds qtpalette dovi_isom isom isom_tags riff riffdec mov_chan
avcodec_sources += ac3tab
avutil_sources += dovi_meta

#ape
avformat_sources += ape
avcodec_sources += apedec lossless_audiodsp bswapdsp

#musepack
avformat_sources += mpc mpc8
avcodec_sources += mpc mpc7 mpc8

#caf
avformat_sources += cafdec caf mov_chan mov_esds
avcodec_sources += pcm

#ogg
avformat_sources += oggdec $(addprefix oggparse,celt dirac flac ogm opus skeleton speex theora vorbis vp8) replaygain \
                 vorbiscomment flac_picture
avcodec_sources += vorbisdec vorbisdsp vorbis vorbis_data vorbis_parser xiph dirac

#asf
avformat_sources += asfdec_f asf asfcrypt asf_tags avlanguage
avcodec_sources += wmalosslessdec wma_common wmaprodec wma wmadec

#alac
avcodec_sources += alac alac_data alacdsp

#tak
avformat_sources += takdec rawdec
avcodec_sources += takdec tak takdsp audiodsp

#xvm
avformat_sources += xwma

#libopus
avcodec_sources += libopus libopusdec opus_parser opus opustab opus_rc
includes.dirs += $(dirs.root)/3rdparty/opus/include

#smacker
avformat_sources += smacker
avcodec_sources += smacker

#riff
avformat_sources += wavdec pcm

#oma
avformat_sources += oma omadec

#msf
avformat_sources += msf

#ac3
avformat_sources += ac3dec
avcodec_sources += ac3 ac3_channel_layout_tab ac3dec_fixed ac3dec ac3dec_data ac3_parser ac3dsp aac_ac3_parser eac3_data

#xma
avcodec_sources += wmaprodec

source_files += $(addprefix libavcodec/,$(avcodec_sources)) $(addprefix libavformat/,$(avformat_sources)) $(addprefix libavutil/,$(avutil_sources))
source_files := $(addsuffix .c,$(sort $(source_files)))

defines += av_restrict=restrict HAVE_AV_CONFIG_H

objects_flat_names := 1

include $(dirs.root)/makefile.mak
