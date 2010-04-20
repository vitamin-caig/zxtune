makebin_name = $(1)
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o
makedir_cmd = mkdir -p $(1)

compiler=gcc

#built-in features
support_oss = 1
support_alsa = 1
#support_sdl = 1

#additional include directories which are intended to be standard
include_dirs += /usr/include/qt4

linux_libraries += $(foreach lib,$(boost_libraries),boost_$(lib)-mt)
