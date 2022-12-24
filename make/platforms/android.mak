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

host=linux
compiler=clang
android.toolchain = $(android.ndk)/toolchains/llvm/prebuilt/linux-x86_64/bin
tools.cxx ?= $(tools.cxxwrapper) $(android.toolchain)/clang++
tools.cc ?= $(tools.ccwrapper) $(android.toolchain)/clang
tools.ld ?= $(android.toolchain)/clang++
tools.ar ?= $(android.toolchain)/llvm-ar
postlink_cmd = true

ifndef profile
ifdef release
android.ld.flags += -Wl,-O3,--gc-sections
endif
endif

libraries.android += c m
