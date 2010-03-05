makebin_name = $(1)
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o
makedir_cmd = mkdir -p $(1)

compiler=gcc
CXX=${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-g++
cxx_flags += --sysroot=${TOOLCHAIN_PATH} -B${TOOLCHAIN_PATH} -mips32 \
  -D'WCHAR_MIN=(0)' -D'WCHAR_MAX=((8 << sizeof(wchar_t)) - 1)'
LDD=${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-ld
ld_flags += --sysroot=${TOOLCHAIN_PATH} -L${TOOLCHAIN_PATH}/usr/mipsel-linux/lib -lstdc++ -lgcc_s
AR=${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-ar
OBJCOPY=${TOOLCHAIN_PATH}/usr/bin/mipsel-linux-objcopy

#built-in features
support_oss = 1

#support only static multithread libraries
linux_libraries += $(foreach lib,$(boost_libraries),boost_$(lib))
