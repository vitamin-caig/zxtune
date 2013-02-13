ifneq ($($(platform).$(arch).boost.version),)
boost.version = $($(platform).$(arch).boost.version)
boost.includes = $(prebuilt.dir)/boost-$(boost.version)
boost.libs = $(prebuilt.dir)/boost-$(boost.version)-$(platform)-$(arch)/lib
endif
include_dirs += $(boost.includes)
$(platform)_libraries_dirs += $(boost.libs)
