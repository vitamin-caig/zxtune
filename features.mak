ifdef support_waveout
definitions += WIN32_WAVEOUT_SUPPORT
$(platform)_libraries += winmm.lib
endif

ifdef support_oss
definitions += OSS_SUPPORT
endif
