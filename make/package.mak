#package generating
pkg_revision := $(subst :,_,$(shell svnversion $(path_step)))
pkg_subversion := $(if $(release),,_dbg)
pkg_suffix := zip

pkg_dir := $(path_step)/Builds/Revision$(pkg_revision)_$(platform)$(if $(arch),_$(arch),)
pkg_filename := $(binary_name)_r$(pkg_revision)$(pkg_subversion)_$(platform)_$(arch).$(pkg_suffix)
pkg_file := $(pkg_dir)/$(pkg_filename)
pkg_log := $(pkg_dir)/packaging_$(binary_name).log
pkg_build_log := $(pkg_dir)/$(binary_name).log
pkg_debug := $(pkg_dir)/$(binary_name)_debug.$(pkg_suffix)

temp_pkg = $(pkg_dir)/pkg

package: | $(pkg_dir)
	@$(MAKE) -s clean_package
	$(info Creating package for $(binary_name) at $(pkg_dir))
	@$(MAKE) $(pkg_file) > $(pkg_log) 2>&1
	@$(call rmfiles_cmd,$(pkg_manual) $(pkg_build_log))

$(pkg_dir):
	$(call makedir_cmd,$@)

$(temp_pkg):
	$(call makedir_cmd,$@)

.PHONY: clean_package

clean_package:
	-$(call rmfiles_cmd,$(pkg_file) $(pkg_debug) $(pkg_build_log) $(pkg_log))
	-$(call rmdir_cmd,$(temp_pkg))

$(pkg_file): $(pkg_debug) $(pkg_additional_files) $(pkg_additional_files_$(platform)) | $(temp_pkg)
	@$(call showtime_cmd)
	$(info Packaging $(binary_name) to $(pkg_filename))
	@$(MAKE) DESTDIR=$(temp_pkg) install
	$(call makepkg_cmd,$(temp_pkg),$@)
	$(call rmdir_cmd,$(temp_pkg))

$(pkg_debug): $(pkg_build_log)
	@$(call showtime_cmd)
	$(info Packaging debug information and build log)
	zip -9Dj $@ $(target).pdb $(pkg_build_log)

$(pkg_build_log):
	@$(call showtime_cmd)
	$(info Building $(binary_name))
	$(MAKE) defines="ZXTUNE_VERSION=rev$(pkg_revision)" > $(pkg_build_log) 2>&1
