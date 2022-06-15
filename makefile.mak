include $(dirs.root)/make/default.mak

ifneq ($(multiarch),)
include $(dirs.root)/make/build_multiarch.mak
else
include $(dirs.root)/make/build.mak
endif
