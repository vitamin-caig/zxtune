ifndef distro
boost.version = $($(platform).$(arch).boost.version)
endif

ifneq ($(boost.version),)
boost.includes = $(prebuilt.dir)/boost-$(boost.version)/include
boost.libs = $(prebuilt.dir)/boost-$(boost.version)-$(platform)-$(arch)/lib
endif
includes.dirs += $(boost.includes)
libraries.dirs.$(platform) += $(boost.libs)

defines += HAVE_BOOST
libraries += $(foreach lib,$(libraries.boost),boost_$(lib))
