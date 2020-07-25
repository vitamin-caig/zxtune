boost.version.merged = $(firstword $($(platform).$(arch).boost.version) $($(platform).boost.version) $(boost.version))

ifneq ($(boost.version.merged),)
includes.dirs += $(prebuilt.dir)/boost-$(boost.version.merged)/include
libraries.dirs.$(platform) += $(prebuilt.dir)/boost-$(boost.version.merged)-$(platform)-$(arch)/lib
endif

libraries += $(foreach lib,$(libraries.boost),boost_$(lib))
