#apply optional local settings
-include $(path_step)/variables.mak

ifndef platform
$(error Required to define 'platform=windows/mingw/linux/etc')
endif

#default language
pkg_lang ?= en
