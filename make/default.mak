#set default parameters
platform ?= $(if $(MINGW_ROOT),mingw,$(if $(VS_PATH),windows,linux))
pkg_lang ?= en
gui_theme ?= default