makebin_name = $(1)
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o

host=linux
compiler?=gcc

ifneq ($($(platform).$(arch).crossroot),)
libraries.dirs.linux += $($(platform).$(arch).crossroot)/usr/lib
includes.dirs.linux += $($(platform).$(arch).crossroot)/usr/include
$(platform).ld.flags += -Wl,--unresolved-symbols=ignore-in-shared-libs
endif

ifdef use_qt
libraries.dirs.linux += $($(platform).$(arch).qt.libs)
includes.dirs.linux += $($(platform).$(arch).qt.includes)
libraries.linux += $($(platform).$(arch).qt.libraries)
endif
