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

#built-in features
support_oss = 1
support_alsa = 1
support_sdl = 1
support_mp3 = 1
support_ogg = 1
support_flac = 1

dynamic_libs += dl pthread

ifdef STATIC_BOOST_PATH
include_dirs += $(STATIC_BOOST_PATH)/include
static_libs_dir += $(STATIC_BOOST_PATH)/lib
static_libraries += $(foreach lib,$(boost_libraries),boost_$(lib)-mt)
static_boost = 1
endif

ifdef static_boost
static_libs += $(foreach lib,$(boost_libraries),boost_$(lib))
else
dynamic_libs += $(foreach lib,$(boost_libraries),boost_$(lib))
endif

ifdef static_qt
static_libs += $(foreach lib,$(qt_libraries),Qt$(lib))
else
dynamic_libs += $(foreach lib,$(qt_libraries),Qt$(lib))
endif
