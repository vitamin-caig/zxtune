library_name := sound
path_step := ../..
source_dirs := impl
source_files = backends/backend_impl backends/enumerator backends/file_backend backends/scope

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

source_files += backends/oss_backend backends/alsa_backend backends/sdl_backend backends/ogg_backend \
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

ifdef support_oss
$(platform)_definitions += OSS_SUPPORT
endif

ifdef support_alsa
$(platform)_definitions += ALSA_SUPPORT
endif

ifdef support_aylpt_dlportio
$(platform)_definitions += DLPORTIO_AYLPT_SUPPORT
endif

ifdef support_sdl
$(platform)_definitions += SDL_SUPPORT
endif

ifdef support_ogg
$(platform)_definitions += OGG_SUPPORT
$(platform)_include_dirs += $(path_step)/3rdparty
$(platform)_include_dirs += $(path_step)/3rdparty
endif
