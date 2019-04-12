library_name := sound_backends
dirs.root := ../../..
source_files = backend_impl.cpp file_backend.cpp service.cpp volume_control.cpp

text_files := text/backends
po_files := sound_backends

include $(dirs.root)/make/default.mak
include $(dirs.root)/make/features.mak

define declare_native_backend
ifdef support_$(1)
generated_headers += gates/$(2)_api.h
generated_sources += gates/$(2)_api_dynamic.cpp
source_files += $(2)_backend.cpp
else
source_files += $(2)_stub.cpp
endif
endef

define declare_thirdparty_backend
ifdef support_$(1)
includes.dirs.$$(platform) += $$(dirs.root)/3rdparty
generated_headers += gates/$(1)_api.h
generated_sources += gates/$(1)_api_dynamic.cpp
source_files += $(1)_backend.cpp
else
source_files += $(1)_stub.cpp
endif
endef

$(eval $(call declare_native_backend,waveout,win32))
$(eval $(call declare_native_backend,directsound,dsound))
$(eval $(call declare_native_backend,alsa,alsa))
$(eval $(call declare_native_backend,sdl,sdl))
$(eval $(call declare_native_backend,pulseaudio,paudio))
$(eval $(call declare_native_backend,openal,openal))

ifdef support_aylpt
source_files += aylpt_backend.cpp
else
source_files += aylpt_stub.cpp
endif

ifdef support_oss
source_files += oss_backend.cpp
else
source_files += oss_stub.cpp
endif

$(eval $(call declare_thirdparty_backend,flac))
$(eval $(call declare_thirdparty_backend,mp3))
$(eval $(call declare_thirdparty_backend,ogg))

ifdef support_ogg
generated_headers += gates/vorbis_api.h gates/vorbisenc_api.h
generated_sources += gates/vorbis_api_dynamic.cpp gates/vorbisenc_api_dynamic.cpp
endif

source_files += null_backend.cpp wav_backend.cpp

include $(dirs.root)/makefile.mak
include $(dirs.root)/make/shgate.mak
