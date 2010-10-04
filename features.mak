ifdef support_waveout
definitions += WIN32_WAVEOUT_SUPPORT
$(platform)_libraries += winmm
endif

ifdef support_oss
definitions += OSS_SUPPORT
endif

ifdef support_alsa
definitions += ALSA_SUPPORT
$(platform)_libraries += asound
endif

ifdef support_aylpt_dlportio
definitions += DLPORTIO_AYLPT_SUPPORT
endif

ifdef support_sdl
definitions += SDL_SUPPORT
$(platform)_libraries += SDL
mingw_libraries += winmm gdi32 dxguid
endif
