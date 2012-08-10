library_name := sound
path_step := ../..
source_dirs := impl
source_files = backends/backend_impl backends/enumerator backends/file_backend backends/scope backends/volume_control

text_files := text/sound text/backends

include $(path_step)/make/default.mak
include $(path_step)/make/features.mak

define declare_native_backend
ifdef support_$(1)
generated_headers += backends/$(2)_api
generated_sources += backends/$(2)_api_dynamic
source_files += backends/$(2)_backend
else
source_files += backends/$(2)_stub
endif
endef

define declare_thirdparty_backend
ifdef support_$(1)
$$(platform)_include_dirs += $$(path_step)/3rdparty
generated_headers += backends/$(1)_api
generated_sources += backends/$(1)_api_dynamic
source_files += backends/$(1)_backend
else
source_files += backends/$(1)_stub
endif
endef

$(eval $(call declare_native_backend,waveout,win32))
$(eval $(call declare_native_backend,directsound,dsound))
$(eval $(call declare_native_backend,alsa,alsa))
$(eval $(call declare_native_backend,sdl,sdl))

ifdef support_oss
source_files += backends/oss_backend
else
source_files += backends/oss_stub
endif

$(eval $(call declare_thirdparty_backend,flac))
$(eval $(call declare_thirdparty_backend,mp3))
$(eval $(call declare_thirdparty_backend,ogg))

ifdef support_ogg
generated_headers += backends/vorbis_api backends/vorbisenc_api
source_files += backends/vorbis_api_dynamic backends/vorbisenc_api_dynamic
endif

source_files += backends/null_backend backends/wav_backend

include $(path_step)/makefile.mak

%_api_dynamic$(src_suffix): templates/%_api_dynamic$(src_suffix).templ functions/%.dat
	perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

%_api.h: templates/%_api.h.templ functions/%.dat
	perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@
