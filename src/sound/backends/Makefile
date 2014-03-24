library_name := sound_backends
path_step := ../../..
source_files = backend_impl file_backend service volume_control

text_files := text/backends
po_files := sound_backends

include $(path_step)/make/default.mak
include $(path_step)/make/features.mak

define declare_native_backend
ifdef support_$(1)
generated_headers += gates/$(2)_api
generated_sources += gates/$(2)_api_dynamic
source_files += $(2)_backend
else
source_files += $(2)_stub
endif
endef

define declare_thirdparty_backend
ifdef support_$(1)
$$(platform)_include_dirs += $$(path_step)/3rdparty
generated_headers += gates/$(1)_api
generated_sources += gates/$(1)_api_dynamic
source_files += $(1)_backend
else
source_files += $(1)_stub
endif
endef

$(eval $(call declare_native_backend,waveout,win32))
$(eval $(call declare_native_backend,directsound,dsound))
$(eval $(call declare_native_backend,alsa,alsa))
$(eval $(call declare_native_backend,sdl,sdl))

ifdef support_oss
source_files += oss_backend
else
source_files += oss_stub
endif

$(eval $(call declare_thirdparty_backend,flac))
$(eval $(call declare_thirdparty_backend,mp3))
$(eval $(call declare_thirdparty_backend,ogg))

ifdef support_ogg
generated_headers += gates/vorbis_api gates/vorbisenc_api
generated_sources += gates/vorbis_api_dynamic gates/vorbisenc_api_dynamic
endif

source_files += null_backend wav_backend

include $(path_step)/makefile.mak
include $(path_step)/make/shgate.mak
