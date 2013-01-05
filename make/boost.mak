prebuilt_dir ?= $(path_step)/../Build

ifneq ($($(platform).$(arch).boost.version),)
boost.version = $($(platform).$(arch).boost.version)
boost.dir = $(prebuilt_dir)/boost-$(boost.version)-$(platform)-$(arch)
include_dirs += $(boost.dir)/include
$(platform)_libraries_dirs += $(boost.dir)/lib
endif
