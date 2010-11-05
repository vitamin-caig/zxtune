ifdef support_waveout
$(platform)_definitions += WIN32_WAVEOUT_SUPPORT
$(platform)_libraries += winmm
endif

ifdef support_oss
$(platform)_definitions += OSS_SUPPORT
endif

ifdef support_alsa
$(platform)_definitions += ALSA_SUPPORT
$(platform)_libraries += asound
endif

ifdef support_aylpt_dlportio
$(platform)_definitions += DLPORTIO_AYLPT_SUPPORT
endif

ifdef support_sdl
$(platform)_definitions += SDL_SUPPORT
$(platform)_libraries += SDL
mingw_libraries += winmm gdi32 dxguid
endif
