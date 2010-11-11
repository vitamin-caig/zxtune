makebin_name = $(1).dge
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o
makedir_cmd = mkdir -p $(1)
rmdir_cmd = rm -Rf $(1)
rmfiles_cmd = rm -f $(1)
showtime_cmd = date +"%x %X"

compiler=gcc
CXX = ${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-g++
CXX_PLATFORM_FLAGS = --sysroot=${TOOLCHAIN_PATH} -B${TOOLCHAIN_PATH} -mips32
LDD = $(CXX)
LD_PLATFORM_FLAGS = --sysroot=${TOOLCHAIN_PATH}
AR = ${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-ar
OBJCOPY = ${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-objcopy
STRIP = ${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-strip

#defines without spaces
dingux_definitions = 'WCHAR_MIN=(0)' 'WCHAR_MAX=((8<<sizeof(wchar_t))-1)'
dingux_libraries = stdc++ gcc c m dl pthread
dingux_libraries_dirs = ${TOOLCHAIN_PATH}/usr/mipsel-linux/lib

#built-in features
support_oss = 1
#support_sdl = 1

ifdef boost_libraries
#support only static multithread libraries
dingux_libraries += $(foreach lib,$(boost_libraries),boost_$(lib))
endif

ifdef qt_libraries
dingux_libraries += $(foreach lib,$(qt_libraries),Qt$(lib))
endif
