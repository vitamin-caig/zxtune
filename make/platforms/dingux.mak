makebin_name = $(1).dge
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o

host=linux
compiler=gcc
CXX = ${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-g++
CC = ${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-cc
CXX_PLATFORM_FLAGS = --sysroot=${TOOLCHAIN_PATH} -B${TOOLCHAIN_PATH} -mips32 -fvisibility=hidden -fvisibility-inlines-hidden
LDD = $(CXX)
LD_PLATFORM_FLAGS = --sysroot=${TOOLCHAIN_PATH}
ifdef release
LD_PLATFORM_FLAGS += -Wl,-O3,-x,--gc-sections,--relax
endif
AR = ${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-ar
OBJCOPY = ${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-objcopy
STRIP = ${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-strip

#defines without spaces
dingux_definitions += 'WCHAR_MIN=(0)' 'WCHAR_MAX=((1<<(8*sizeof(wchar_t)))-1)' 'BOOST_FILESYSTEM_VERSION=2'
dingux_libraries += stdc++ gcc c m dl pthread
dingux_libraries_dirs += ${TOOLCHAIN_PATH}/usr/mipsel-linux/lib

#built-in features
support_oss = 1
#support_sdl = 1

ifdef STATIC_BOOST_PATH
include_dirs += $(STATIC_BOOST_PATH)/include
dingux_libraries_dirs += $(STATIC_BOOST_PATH)/lib
endif

#support only static multithread release libraries
dingux_libraries += $(foreach lib,$(boost_libraries),boost_$(lib))

#support static release libraries
dingux_libraries += $(foreach lib,$(qt_libraries),Qt$(lib))
