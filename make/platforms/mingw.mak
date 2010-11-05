makebin_name = $(1).exe
makelib_name = lib$(1).a
makedyn_name = $(1).dll
makeobj_name = $(1).o
makeres_name = $(1).res
makeres_cmd = windres -O coff $(addprefix -D, $(definitions)) $(1) $(2)
makedir_cmd = if NOT EXIST $(subst /,\,$(1)) mkdir $(subst /,\,$(1))
rmdir_cmd = rmdir /Q /S $(subst /,\,$(1))
rmfiles_cmd = del /Q $(subst /,\,$(1))
showtime_cmd = echo %TIME%

compiler=gcc
CXX_PLATFORM_FLAGS = -mthreads -march=native
LD_PLATFORM_FLAGS = -mthreads -static 
ifdef release
LD_PLATFORM_FLAGS += -Wl,-subsystem,$(ifdef $(qt_libraries),windows,console)
else
LD_PLATFORM_FLAGS += -Wl,-subsystem,console
endif

#built-in features
support_waveout = 1
support_aylpt_dlportio = 1
#support_sdl = 1

#simple library naming convention used
ifdef boost_libraries
mingw_definitions += BOOST_THREAD_USE_LIB
mingw_libraries += $(foreach lib,$(boost_libraries),boost_$(lib))
endif

ifdef qt_libraries
mingw_libraries += $(foreach lib,$(qt_libraries),Qt$(lib))
endif
