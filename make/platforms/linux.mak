makebin_name = $(1)
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o

host=linux
compiler=gcc
CXX_PLATFORM_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
ifdef release
ifndef profile
LD_PLATFORM_FLAGS += -Wl,-O3,-x,--gc-sections,--relax
endif
endif

ifneq ($(findstring $(arch),arm),)
ifeq ($(TOOLCHAIN_PATH),)
TOOLCHAIN_PATH=/usr
endif
ifneq ($(TOOLCHAIN_ROOT),)
CXX_PLATFORM_FLAGS += -I${TOOLCHAIN_ROOT}/usr/include
LD_PLATFORM_FLAGS += -L${TOOLCHAIN_ROOT}/usr/lib -Wl,--unresolved-symbols=ignore-in-shared-libs
endif
CXX = ${TOOLCHAIN_PATH}/bin/arm-none-linux-gnueabi-g++
LDD = ${TOOLCHAIN_PATH}/bin/arm-none-linux-gnueabi-g++
CC = ${TOOLCHAIN_PATH}/bin/arm-none-linux-gnueabi-gcc
AR = ${TOOLCHAIN_PATH}/bin/arm-none-linux-gnueabi-ar
OBJCOPY = ${TOOLCHAIN_PATH}/bin/arm-none-linux-gnueabi-objcopy
STRIP = ${TOOLCHAIN_PATH}/bin/arm-none-linux-gnueabi-strip
endif

$(platform)_libraries += dl rt pthread stdc++

ifdef STATIC_BOOST_PATH
include_dirs += $(STATIC_BOOST_PATH)/include
$(platform)_libraries_dirs += $(STATIC_BOOST_PATH)/lib
boost_libs_model = -mt
endif

#multithread release libraries
$(platform)_libraries += $(foreach lib,$(boost_libraries),boost_$(lib)$(boost_libs_model))

#release libraries
$(platform)_libraries += $(foreach lib,$(qt_libraries),Qt$(lib))
