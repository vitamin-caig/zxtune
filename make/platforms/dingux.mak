makebin_name = $(1).dge
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o

host=linux
compiler=gcc
CXX = $($(platform).$(arch).execprefix)g++
CC = $($(platform).$(arch).execprefix)gcc
LDD = $($(platform).$(arch).execprefix)g++
AR = $($(platform).$(arch).execprefix)ar
OBJCOPY = $($(platform).$(arch).execprefix)objcopy
STRIP = $($(platform).$(arch).execprefix)strip
CXX_PLATFORM_FLAGS = --sysroot=$($(platform).$(arch).toolchain) -B$($(platform).$(arch).toolchain) -mips32 -fvisibility=hidden -fvisibility-inlines-hidden
LD_PLATFORM_FLAGS = --sysroot=$($(platform).$(arch).toolchain)
ifdef release
LD_PLATFORM_FLAGS += -Wl,-O3,-x,--gc-sections,--relax
endif

#defines without spaces
dingux_definitions += 'WCHAR_MIN=(0)' 'WCHAR_MAX=((1<<(8*sizeof(wchar_t)))-1)' 'BOOST_FILESYSTEM_VERSION=2'
dingux_libraries += stdc++ gcc c m dl pthread
#dingux_libraries_dirs += ${TOOLCHAIN_PATH}/usr/mipsel-linux/lib

#support only static multithread release libraries
dingux_libraries += $(foreach lib,$(boost_libraries),boost_$(lib))

#support static release libraries
dingux_libraries += $(foreach lib,$(qt_libraries),Qt$(lib))
