#package generating
include $(path_step)/make/version.mak

pkg_version := $(root.version)$(if $(debug),_dbg,)

pkg_name ?= $(binary_name)
packaging ?= any
pkg_tagged_name := $(pkg_name)_$(packaging)$(if $(distro),_$(distro),)
pkg_dir := $(path_step)/Builds/$(pkg_version)/$(platform)$(if $(arch),/$(arch),)
pkg_build_log := $(pkg_dir)/$(pkg_tagged_name).log
pkg_debug := $(pkg_dir)/$(pkg_tagged_name)_debug.$(pkg_suffix)

pkg_debug_root = $(pkg_dir)/debug

package: | $(pkg_dir)
	$(info Building package $(pkg_name))
	@$(MAKE) $(pkg_debug)
	@$(MAKE) package_$(packaging)

$(pkg_debug): | $(pkg_debug_root)
	@$(call showtime_cmd)
	$(MAKE) DESTDIR=$(pkg_debug_root) install_debug
	$(info Packaging debug information and build log)
	$(call makepkg_cmd,$(pkg_debug_root),$@)
	$(call rmdir_cmd,$(pkg_debug_root))

$(pkg_build_log): | $(pkg_dir)
	@$(call showtime_cmd)
	$(info Compile $(pkg_name))
	$(MAKE) $(if $(cpu.cores),-j $(cpu.cores),) > $(pkg_build_log) 2>&1

ifdef target
install_debug: $(pkg_build_log)
	$(if $(target),$(call copyfile_cmd,$(target).pdb,$(DESTDIR)),)
	$(call copyfile_cmd,$(pkg_build_log),$(DESTDIR))
	$(call rmfiles_cmd,$(pkg_build_log))
endif

$(pkg_dir) $(pkg_debug_root):
	$(call makedir_cmd,$@)

include $(path_step)/make/package_$(packaging).mak
