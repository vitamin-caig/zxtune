makebin_name = $(1).exe
makelib_name = lib$(1).a
makedyn_name = $(1).dll
makeobj_name = $(1).o
makeres_name = $(1).res
makeres_cmd = windres -O coff $(addprefix -D, $(DEFINITIONS)) $(1) $(2)

host=windows
compiler=gcc
CXX_PLATFORM_FLAGS = -mthreads -march=i686 -mtune=generic -m32 -mmmx
LD_PLATFORM_FLAGS = -mthreads -static -Wl,--allow-multiple-definition
ifdef release
CXX_PLATFORM_FLAGS += -minline-all-stringops
ifndef profile
LD_PLATFORM_FLAGS += -Wl,-O3,-x,--gc-sections,--relax,--kill-at
endif
endif

ifdef use_qt
LD_PLATFORM_FLAGS += -Wl,-subsystem,$(if $(release),windows,console)
else
LD_PLATFORM_FLAGS += -Wl,-subsystem,console
endif

mingw_definitions += BOOST_THREAD_USE_LIB BOOST_FILESYSTEM_VERSION=3

#built-in features
support_waveout = 1
support_aylpt_dlportio = 1
support_directsound = 1
support_sdl = 1
support_zlib = 1

#simple library naming convention used
mingw_libraries += $(foreach lib,$(boost_libraries),boost_$(lib))

mingw_libraries += $(foreach lib,$(qt_libraries),Qt$(lib))

