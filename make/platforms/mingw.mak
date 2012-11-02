makebin_name = $(1).exe
makelib_name = lib$(1).a
makedyn_name = $(1).dll
makeobj_name = $(1).o
makeres_name = $(1).res
makeres_cmd = windres -O coff --input $(1) --output $(2) $(addprefix -D,$(DEFINITIONS))
ifeq ($(BUILD_ARCH),x86_64)
arch64 := 1
makeres_cmd += -F pe$(if $(pic),i,)-x86-64 -O coff -DMANIFEST_NAME=$(platform)_$(BUILD_ARCH).manifest
else
makeres_cmd += -F pe$(if $(pic),i,)-i386
endif

host=windows
compiler=gcc
CXX = g++
CC = gcc
CXX_PLATFORM_FLAGS = -mthreads -mwin32 -mno-ms-bitfields $(if $(arch64),-m64,-m32) -mmmx -msse -msse2
LD_PLATFORM_FLAGS = -mthreads -static -Wl,--allow-multiple-definition $(if $(arch64),-m64,-m32)
ifdef release
CXX_PLATFORM_FLAGS += -minline-all-stringops
ifndef profile
LD_PLATFORM_FLAGS += -Wl,-O3,-x,--gc-sections,--relax,--kill-at
endif
endif

ifdef have_gui
LD_PLATFORM_FLAGS += -Wl,-subsystem,windows
else
LD_PLATFORM_FLAGS += -Wl,-subsystem,console
endif

mingw_definitions += BOOST_THREAD_USE_LIB BOOST_FILESYSTEM_VERSION=3

#simple library naming convention used
mingw_libraries += $(foreach lib,$(boost_libraries),boost_$(lib)-mt$(if $(release),,-d))

mingw_libraries += $(foreach lib,$(qt_libraries),Qt$(lib)$(if $(release),,d))

