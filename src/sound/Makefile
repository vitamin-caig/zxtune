library_name := sound
path_step := ../..
source_dirs := impl backends

text_files := text/sound text/backends

include $(path_step)/makefile.mak

ifdef support_waveout
$(platform)_definitions += WIN32_WAVEOUT_SUPPORT
endif

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

ifdef support_directsound
$(platform)_definitions += DIRECTSOUND_SUPPORT
endif

ifdef support_mp3
$(platform)_definitions += MP3_SUPPORT
windows_include_dirs += $(path_step)/3rdparty
mingw_include_dirs += $(path_step)/3rdparty
endif

ifdef support_ogg
$(platform)_definitions += OGG_SUPPORT
windows_include_dirs += $(path_step)/3rdparty
mingw_include_dirs += $(path_step)/3rdparty
endif

ifdef support_flac
$(platform)_definitions += FLAC_SUPPORT
windows_include_dirs += $(path_step)/3rdparty
mingw_include_dirs += $(path_step)/3rdparty
endif
