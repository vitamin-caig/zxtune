#set default parameters

#guess platform if not set
platform ?= $(if $(MINGW_DIR),mingw,$(if $(VS_PATH),windows,linux))
#set architecture from environment
arch ?= $(BUILD_ARCH)
#default language
pkg_lang ?= en
