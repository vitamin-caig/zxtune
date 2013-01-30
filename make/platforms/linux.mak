makebin_name = $(1)
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o

host=linux
compiler=gcc
CXX_PLATFORM_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden $($(platform).$(arch).cxx.flags)
CXX = $($(platform).$(arch).execprefix)g++
CC = $($(platform).$(arch).execprefix)gcc
LDD = $($(platform).$(arch).execprefix)g++
AR = $($(platform).$(arch).execprefix)ar
OBJCOPY = $($(platform).$(arch).execprefix)objcopy
STRIP = $($(platform).$(arch).execprefix)strip
ifdef release
ifndef profile
LD_PLATFORM_FLAGS += -Wl,-O3,-x,--gc-sections,--relax
endif
endif

$(platform)_libraries += dl rt pthread stdc++

ifneq ($($(platform).$(arch).crossroot),)
$(platform)_libraries_dirs += $($(platform).$(arch).crossroot)/usr/lib
$(platform)_include_dirs += $($(platform).$(arch).crossroot)/usr/include
LD_PLATFORM_FLAGS += -Wl,--unresolved-symbols=ignore-in-shared-libs
endif

ifdef use_qt
$(platform)_libraries_dirs += $($(platform).$(arch).qt.libs)
$(platform)_include_dirs += $($(platform).$(arch).qt.includes)
$(platform)_libraries += $($(platform).$(arch).qt.libraries)
endif

#multithread release libraries
$(platform)_libraries += $(foreach lib,$(boost_libraries),boost_$(lib)$($(platform).$(arch).boost.libs.model))

#release libraries
$(platform)_libraries += $(foreach lib,$(qt_libraries),Qt$(lib))
