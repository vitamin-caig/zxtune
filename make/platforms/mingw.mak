ifeq ($(BUILD_ARCH),x86_64)
arch64 := 1
endif
makebin_name = $(1).exe
makelib_name = lib$(1).a
makedyn_name = $(1).dll
makeobj_name = $(1).o
makeres_name = $(1).res
makeres_cmd = windres -F pe$(if $(pic),i,)-$(if $(arch64),x86-64,i386) -O coff $(addprefix -D, $(DEFINITIONS)) $(1) $(2)

host=windows
compiler=gcc
CXX_PLATFORM_FLAGS = -mthreads -mwin32 -mno-ms-bitfields $(if $(arch64),-m64,-m32) -mmmx -msse -msse2
LD_PLATFORM_FLAGS = -mthreads -static -Wl,--allow-multiple-definition $(if $(arch64),-m64,-m32)
ifdef release
CXX_PLATFORM_FLAGS += -minline-all-stringops
ifndef profile
LD_PLATFORM_FLAGS += -Wl,-O3,-x,--gc-sections,--relax,--kill-at
endif
endif

ifdef use_qt
LD_PLATFORM_FLAGS += -Wl,-subsystem,windows
else
LD_PLATFORM_FLAGS += -Wl,-subsystem,console
endif

mingw_definitions += BOOST_THREAD_USE_LIB BOOST_FILESYSTEM_VERSION=3

#built-in features
support_waveout = 1
support_aylpt_dlportio = 1
support_directsound = 1
#support_sdl = 1
support_zlib = 1
support_mp3 = 1
support_ogg = 1
support_flac = 1

#simple library naming convention used
mingw_libraries += $(foreach lib,$(boost_libraries),boost_$(lib)-mt$(if $(release),,-d))

mingw_libraries += $(foreach lib,$(qt_libraries),Qt$(lib)$(if $(release),,d))

