#package generating
include $(path_step)/make/version.mak

pkg_version := $(root.version)$(if $(debug),_dbg,)

pkg_name ?= $(binary_name)
packaging ?= any
pkg_tagged_name := $(pkg_name)_$(packaging)$(if $(distro),_$(distro),)
pkg_dir := $(path_step)/Builds/$(pkg_version)/$(platform)$(if $(arch),/$(arch),)
pkg_debug := $(pkg_dir)/$(pkg_tagged_name)_debug.$(pkg_suffix)

pkg_debug_root = $(pkg_dir)/debug

package: | $(pkg_dir)
	$(info Building package $(pkg_name))
	@$(MAKE) $(pkg_debug)
	@$(MAKE) package_$(packaging)

$(pkg_debug): | $(pkg_debug_root)
	@$(call showtime_cmd)
	$(MAKE) DESTDIR=$(pkg_debug_root) install_debug
	$(info Packaging debug information)
	$(call makepkg_cmd,$(pkg_debug_root),$@)
	$(call rmdir_cmd,$(pkg_debug_root))

package_targets:
	@$(call showtime_cmd)
	$(info Compile $(pkg_name))
	$(MAKE) $(if $(cpu.cores),-j $(cpu.cores),)

ifdef target
install_debug: package_targets
	$(if $(target),$(call copyfile_cmd,$(target).pdb,$(DESTDIR)),)
endif

$(pkg_dir) $(pkg_debug_root):
	$(call makedir_cmd,$@)

include $(path_step)/make/package_$(packaging).mak
