#package generating
#TODO: make proper revision number width
pkg_revision := 0$(shell svnversion $(path_step))
ifneq ($(pkg_revision),$(subst :,,$(pkg_revision)))
$(error Invalid package revision ($(pkg_revision) - possible mixed))
endif
pkg_subversion := $(if $(subst release,,$(mode)),_dbg,)
pkg_suffix := zip

pkg_dir := $(path_step)/Builds/Revision$(pkg_revision)_$(platform)$(if $(arch),_$(arch),)
pkg_filename := $(binary_name)_r$(pkg_revision)$(pkg_subversion)_$(platform)_$(arch).$(pkg_suffix)
pkg_file := $(pkg_dir)/$(pkg_filename)
pkg_log := $(pkg_dir)/packaging_$(binary_name).log
pkg_build_log := $(pkg_dir)/$(binary_name).log
pkg_debug := $(pkg_dir)/$(binary_name)_debug.$(pkg_suffix)

package: | $(pkg_dir)
	@$(MAKE) -s clean_package
	$(info Creating package at $(pkg_dir))
	@$(MAKE) $(pkg_file) > $(pkg_log) 2>&1
	@$(call rmfiles_cmd,$(pkg_manual) $(pkg_build_log))

$(pkg_dir):
	$(call makedir_cmd,$@)

.PHONY: clean_package

clean_package:
	-$(call rmfiles_cmd,$(pkg_file) $(pkg_debug) $(pkg_log))

$(pkg_file): $(pkg_debug) $(pkg_additional_files) $(pkg_additional_files_$(platform))
	@$(call showtime_cmd)
	$(info Packaging $(binary_name) to $(pkg_filename))
	zip -9Dj $@ $(target) $(pkg_additional_files) $(pkg_additional_files_$(platform)) $(pkg_manual)

$(pkg_debug): $(pkg_build_log)
	@$(call showtime_cmd)
	$(info Packaging debug information and build log)
	zip -9Dj $@ $(target).pdb $(pkg_build_log)

$(pkg_build_log):
	@$(call showtime_cmd)
	$(MAKE) defines="ZXTUNE_VERSION=rev$(pkg_revision) BUILD_PLATFORM=$(platform) BUILD_ARCH=$(arch)" > $(pkg_build_log) 2>&1
