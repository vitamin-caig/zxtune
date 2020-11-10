library_name := sound_backends_winstub
dirs.root := ../../../..
source_files = ../service.cpp ../l10n.cpp ../volume_control.cpp ../null_backend.cpp stub.cpp
text_files := ../text/backends

source_files += $(foreach b,win32 dsound, ../$(b)_backend.cpp ../gates/$(b)_api_dynamic.cpp)
source_files += $(foreach b,alsa aylpt flac mp3 ogg openal oss paudio sdl, ../$(b)_stub.cpp)

jumbo.name := $(library_name)
jumbo.for_generated = 1
jumbo.excludes := ../text/backends.cpp

include $(dirs.root)/makefile.mak
