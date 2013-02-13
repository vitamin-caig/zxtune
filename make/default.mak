#set default parameters

ifndef platform
$(error Required to define 'platform=windows/mingw/linux/dingux/etc')
endif

ifndef arch
$(error Required to define 'arch=x86/x86_64/arm/armhf/mipsel/etc')
endif

#default language
pkg_lang ?= en
