library_name := openmpt
dirs.root := ../..
include $(dirs.root)/make/default.mak

includes.dirs += common . build/svn_version src

source_files += $(addprefix libopenmpt/,\
                  libopenmpt_cxx libopenmpt_impl libopenmpt_ext_impl)

source_files += $(addprefix soundlib/,\
                  modcommand mod_specifications modsmp_ctrl patternContainer pattern tuning tuningCollection \
                  ModChannel ModInstrument ModSample ModSequence \
                  Fastmix UpgradeModule \
                  ITCompression ITTools S3MTools XMTools InstrumentExtensions \
                  Message MIDIMacros MIDIEvents MPEGFrame MixerLoops MixerSettings MixFuncTable \
                  OggStream OPL Paula RowVisitor \
                  SampleIO SampleFormats $(addprefix SampleFormat,BRR FLAC MediaFoundation MP3 Opus SFZ Vorbis) \
                  SoundFilePlayConfig Sndfile Sndmix Snd_flt Snd_fx Tables \
                  $(addprefix Load_,669 amf ams c67 dbm digi dmf dsm dsym dtm far fmt gdm imf it itp mdl med mo3 mod mt2 mtm mus_km okt plm psm ptm s3m sfx stm stp symmod ult xm) \
                  load_j2b \
                  TinyFFT WindowedFIR WAVTools Tagging \
                  )

source_files += $(addprefix soundlib/plugins/,\
                  PlugInterface PluginManager \
                  DigiBoosterEcho LFOPlugin SymMODEcho \
                  )
source_files += $(addprefix soundlib/plugins/dmo/,\
                  Chorus Compressor Distortion Echo Flanger Gargle I3DL2Reverb ParamEq WavesReverb \
                  DMOUtils \
                  )

source_files += $(addprefix sounddsp/,Reverb)

source_files += $(addprefix common/,\
                  Logging mptPathString mptRandom mptString mptStringBuffer mptStringFormat mptStringParse mptTime \
                  serialization_utils version)

source_files := $(addsuffix .cpp,$(source_files))

defines += LIBOPENMPT_BUILD NO_LOGGING NO_MPG123 NO_CONTAINERS_SUPPORT OPENMPT_VERSION_REVISION=0

defines += MPT_WITH_MINIMP3 MPT_WITH_VORBISFILE

# force using 3rdparty externals
includes.dirs.SampleFormatMP3.cpp = $(dirs.root)/3rdparty
includes.dirs.SampleFormatVorbis.cpp = $(dirs.root)/3rdparty
includes.dirs.Load_mo3.cpp = $(dirs.root)/3rdparty
includes.dirs.Load_xm.cpp = $(dirs.root)/3rdparty

# declare ASSERT macro as Require to avoid AssertHandler
defines += MPT_FRAMEWORK_ASSERT_IS_DEFINED MPT_ASSERT_USE_FRAMEWORK=1 MPT_FRAMEWORK_ASSERT_IS_ACTIVE=1 "ASSERT(p)=(void)(p)" MPT_LOG_IS_DISABLED

include $(dirs.root)/makefile.mak
