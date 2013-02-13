makebin_name = $(1)
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o

host=linux
compiler=gcc

$(platform)_libraries += dl rt pthread stdc++

ifneq ($($(platform).$(arch).crossroot),)
$(platform)_libraries_dirs += $($(platform).$(arch).crossroot)/usr/lib
$(platform)_include_dirs += $($(platform).$(arch).crossroot)/usr/include
$(platform).ld.flags += -Wl,--unresolved-symbols=ignore-in-shared-libs
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
