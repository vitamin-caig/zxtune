makebin_name = $(1).dge
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o
makedir_cmd = mkdir -p $(1)

compiler=gcc
CXX=${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-g++
cxx_flags += --sysroot=${TOOLCHAIN_PATH} -B${TOOLCHAIN_PATH} -mips32 \
  -D'WCHAR_MIN=(0)' -D'WCHAR_MAX=((8 << sizeof(wchar_t)) - 1)'
LDD=$(CXX)
ld_flags += --sysroot=${TOOLCHAIN_PATH} -L${TOOLCHAIN_PATH}/usr/mipsel-linux/lib -lstdc++ -lgcc -lc -lm -ldl -lpthread -s
AR=${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-ar
OBJCOPY=${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-objcopy
STRIP=${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-strip

#built-in features
support_oss = 1
#support_sdl = 1

#support only static multithread libraries
dingux_libraries += $(foreach lib,$(boost_libraries),boost_$(lib))
dingux_libraries += $(foreach lib,$(qt_libraries),Qt$(lib))
