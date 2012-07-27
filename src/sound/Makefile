library_name := sound
path_step := ../..
source_dirs := impl
source_files = backends/backend_impl backends/enumerator backends/file_backend backends/scope backends/volume_control

text_files := text/sound text/backends

#TODO: extract these includes
include $(path_step)/make/default.mak
include $(path_step)/make/platforms/$(platform).mak

ifdef support_waveout
generated_headers += backends/win32_api
source_files += backends/win32_backend
generated_sources += backends/win32_api_dynamic
else
source_files += backends/win32_stub
endif

ifdef support_directsound
generated_headers += backends/dsound_api
source_files += backends/dsound_backend
generated_sources += backends/dsound_api_dynamic
else
source_files += backends/dsound_stub
endif

ifdef support_flac
$(platform)_include_dirs += $(path_step)/3rdparty
$(platform)_include_dirs += $(path_step)/3rdparty
generated_headers += backends/flac_api
source_files += backends/flac_backend
generated_sources += backends/flac_api_dynamic
else
source_files += backends/flac_stub
endif

ifdef support_mp3
$(platform)_include_dirs += $(path_step)/3rdparty
$(platform)_include_dirs += $(path_step)/3rdparty
generated_headers += backends/mp3_api
source_files += backends/mp3_backend
generated_sources += backends/mp3_api_dynamic
else
source_files += backends/mp3_stub
endif

ifdef support_ogg
$(platform)_include_dirs += $(path_step)/3rdparty
$(platform)_include_dirs += $(path_step)/3rdparty
generated_headers += backends/ogg_api backends/vorbis_api backends/vorbisenc_api
source_files += backends/ogg_backend
generated_sources += backends/ogg_api_dynamic backends/vorbis_api_dynamic backends/vorbisenc_api_dynamic
else
source_files += backends/ogg_stub
endif

ifdef support_alsa
generated_headers += backends/alsa_api
source_files += backends/alsa_backend
generated_sources += backends/alsa_api_dynamic
else
source_files += backends/alsa_stub
endif

ifdef support_sdl
generated_headers += backends/sdl_api
source_files += backends/sdl_backend
generated_sources += backends/sdl_api_dynamic
else
source_files += backends/sdl_stub
endif

source_files += backends/oss_backend \
                backends/null_backend backends/wav_backend

include $(path_step)/makefile.mak

backends/stubs/alsa.h: backends/functions/alsa.dat
		cat $^ | $(path_step)/make/build_shgate.pl ASOUND_FUNC > $@

backends/win32_api_dynamic.cpp: backends/templates/win32_api_dynamic.cpp.templ backends/functions/win32.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/win32_api.h: backends/templates/win32_api.h.templ backends/functions/win32.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/dsound_api_dynamic.cpp: backends/templates/dsound_api_dynamic.cpp.templ backends/functions/dsound.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/dsound_api.h: backends/templates/dsound_api.h.templ backends/functions/dsound.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/flac_api_dynamic.cpp: backends/templates/flac_api_dynamic.cpp.templ backends/functions/flac.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/flac_api.h: backends/templates/flac_api.h.templ backends/functions/flac.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/mp3_api_dynamic.cpp: backends/templates/mp3_api_dynamic.cpp.templ backends/functions/mp3.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/mp3_api.h: backends/templates/mp3_api.h.templ backends/functions/mp3.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/ogg_api_dynamic.cpp: backends/templates/ogg_api_dynamic.cpp.templ backends/functions/ogg.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/ogg_api.h: backends/templates/ogg_api.h.templ backends/functions/ogg.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/vorbis_api_dynamic.cpp: backends/templates/vorbis_api_dynamic.cpp.templ backends/functions/vorbis.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/vorbis_api.h: backends/templates/vorbis_api.h.templ backends/functions/vorbis.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/vorbisenc_api_dynamic.cpp: backends/templates/vorbisenc_api_dynamic.cpp.templ backends/functions/vorbisenc.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/vorbisenc_api.h: backends/templates/vorbisenc_api.h.templ backends/functions/vorbisenc.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/alsa_api_dynamic.cpp: backends/templates/alsa_api_dynamic.cpp.templ backends/functions/alsa.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/alsa_api.h: backends/templates/alsa_api.h.templ backends/functions/alsa.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/sdl_api_dynamic.cpp: backends/templates/sdl_api_dynamic.cpp.templ backends/functions/sdl.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

backends/sdl_api.h: backends/templates/sdl_api.h.templ backends/functions/sdl.dat
		perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

ifdef support_oss
$(platform)_definitions += OSS_SUPPORT
endif

ifdef support_aylpt_dlportio
$(platform)_definitions += DLPORTIO_AYLPT_SUPPORT
endif
