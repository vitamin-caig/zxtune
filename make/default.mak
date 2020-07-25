#apply optional local settings
-include $(dirs.root)/variables.mak

ifndef platform
$(error Required to define 'platform=windows/mingw/linux/etc')
endif

#default language
pkg_lang ?= en
