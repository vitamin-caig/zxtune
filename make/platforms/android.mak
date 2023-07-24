makebin_name = $(1)
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o

ifndef android.ndk
$(error android.ndk is not defined)
endif

ifeq ($(arch),)
$(error Architecture is not defined)
endif

android.target.x86 = i686-linux-android16
android.target.x86_64 = x86_64-linux-android21
android.target.armeabi-v7a = armv7a-linux-androideabi16 -mthumb
android.target.arm64-v8a = aarch64-linux-android21

host=linux
compiler=clang
android.toolchain = $(android.ndk)/toolchains/llvm/prebuilt/linux-x86_64/bin
android.$(arch).execprefix = $(android.toolchain)/
tools.ar ?= $(android.toolchain)/llvm-ar
postlink_cmd = true

android.cxx.flags += -target $(android.target.$(arch))
android.ld.flags += -target $(android.target.$(arch))

ifndef profile
ifdef release
android.ld.flags += -Wl,-O3,--gc-sections
endif
endif

libraries.android += c m
