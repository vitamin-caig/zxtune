#set default parameters
ifndef platform
platform := $(if $(MINGW_ROOT),mingw,$(if $(VS_PATH),windows,linux))
endif
ifndef mode
mode := debug
endif
ifndef pkg_lang
pkg_lang := en
endif
