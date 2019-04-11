ifndef distro
boost.version = $($(platform).$(arch).boost.version)
else
$(platform).$(arch).boost.libs.model =
endif

ifneq ($(boost.version),)
boost.includes = $(prebuilt.dir)/boost-$(boost.version)/include
boost.libs = $(prebuilt.dir)/boost-$(boost.version)-$(platform)-$(arch)/lib
endif
include_dirs += $(boost.includes)
libraries.dirs.$(platform) += $(boost.libs)

defines += HAVE_BOOST
