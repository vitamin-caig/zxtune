ifndef distro
boost.version = $($(platform).$(arch).boost.version)
endif

ifneq ($(boost.version),)
boost.includes = $(prebuilt.dir)/boost-$(boost.version)/include
boost.libs = $(prebuilt.dir)/boost-$(boost.version)-$(platform)-$(arch)/lib
endif
include_dirs += $(boost.includes)
$(platform)_libraries_dirs += $(boost.libs)
