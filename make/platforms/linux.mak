makebin_name = $(1)
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o
makedir_cmd = mkdir -p $(1)
rmdir_cmd = rm -Rf $(1)
rmfiles_cmd = rm -f $(1)
showtime_cmd = date +"%x %X"

compiler=gcc
CXX_PLATFORM_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden

#built-in features
support_oss = 1
support_alsa = 1
#support_sdl = 1

ifdef boost_libraries
linux_libraries += $(foreach lib,$(boost_libraries),boost_$(lib)-mt)
endif

ifdef qt_libraries
linux_libraries += $(foreach lib,$(qt_libraries),Qt$(lib))
include_dirs += /usr/include/qt4
endif
