makebin_name = $(1)
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o

ifeq ($(arch),x86)
else ifeq ($(arch),armeabi)
CXX_PLATFORM_FLAGS += -march=armv5te -mtune=xscale -msoft-float -mthumb
else ifeq ($(arch),armeabi-v7a)
CXX_PLATFORM_FLAGS += -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb
LD_PLATFORM_FLAGS += -march=armv7-a -Wl,--fix-cortex-a8
else ifeq ($(arch),mips)
CXX_PLATFORM_FLAGS += -fno-inline-functions-called-once -fgcse-after-reload -frerun-cse-after-loop -frename-registers 
else
$(error Unknown android architecture. Possible values are x86 armeabi armeabi-v7a mips)
endif

host=linux
compiler=gcc
CXX = $($(platform).$(arch).execprefix)g++
CC = $($(platform).$(arch).execprefix)gcc
LDD = $($(platform).$(arch).execprefix)g++
AR = $($(platform).$(arch).execprefix)ar
OBJCOPY = $($(platform).$(arch).execprefix)objcopy
STRIP = $($(platform).$(arch).execprefix)strip

CXX_PLATFORM_FLAGS += -no-canonical-prefixes -fvisibility-inlines-hidden -funwind-tables -fstack-protector -fomit-frame-pointer -finline-limit=300 -Wa,--noexecstack 
LD_PLATFORM_FLAGS +=  -no-canonical-prefixes -Wl,-soname,$(notdir $@) -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now
ifdef release
LD_PLATFORM_FLAGS += -Wl,-O3,-x,--gc-sections,--relax
endif

#assume that all the platforms are little-endian
#this required to use boost which doesn't know anything about __armel__ or __mipsel__
$(platform)_definitions += ANDROID __ANDROID__ __LITTLE_ENDIAN__ 'BOOST_FILESYSTEM_VERSION=2'
ifneq ($(findstring $(arch),armeabi),)
$(platform)_definitions += __ARM_ARCH_5__ __ARM_ARCH_5T__ __ARM_ARCH_5E__ __ARM_ARCH_5TE__
endif
$(platform)_libraries += c m
