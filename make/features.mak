ifneq ($(findstring $(platform),windows mingw),)
support_waveout = 1
support_aylpt = 1
support_directsound = 1
#support_sdl = 1
support_mp3 = 1
support_ogg = 1
support_flac = 1
support_curl = 1
else ifneq ($(findstring $(platform),linux),)
support_oss = 1
support_alsa = 1
#support_sdl = 1
support_pulseaudio = 1
support_mp3 = 1
support_ogg = 1
support_flac = 1
support_curl = 1
else ifneq ($(findstring $(platform),darwin),)
support_openal = 1
support_mp3 = 1
support_ogg = 1
support_flac = 1
support_curl = 1
else ifeq ($(platform),android)
#no features
else ifeq ($(platform),diagnostics)
support_waveout = 1
support_aylpt = 1
support_directsound = 1
support_sdl = 1
support_mp3 = 1
support_ogg = 1
support_flac = 1
support_curl = 1
support_oss = 1
support_alsa = 1
support_pulseaudio = 1
support_openal = 1
else
$(warning Unknown platform)
endif
