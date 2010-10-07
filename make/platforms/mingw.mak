makebin_name = $(1).exe
makelib_name = lib$(1).a
makedyn_name = $(1).dll
makeobj_name = $(1).o
makedir_cmd = if NOT EXIST $(subst /,\,$(1)) mkdir $(subst /,\,$(1))
rmdir_cmd = rmdir /Q /S $(subst /,\,$(1))
rmfiles_cmd = del /Q $(subst /,\,$(1))

compiler=gcc
cxx_mode_flags += -mthreads -march=native
ld_mode_flags += -mthreads -static 
ifeq ($(mode),release)
ld_mode_flags += -Wl,-subsystem,$(if $(qt_libraries),windows,console)
else
ld_mode_flags += -Wl,-subsystem,console
endif

#built-in features
support_waveout = 1
support_aylpt_dlportio = 1
#support_sdl = 1

#to support latest boost building scheme
definitions += BOOST_THREAD_USE_LIB

#simple library naming convention used
mingw_libraries += $(foreach lib,$(boost_libraries),boost_$(lib))
mingw_libraries := $(foreach lib,$(qt_libraries),Qt$(lib)) $(mingw_libraries)
