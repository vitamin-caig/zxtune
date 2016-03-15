pkg_file = $(pkg_dir)/$(pkg_name)_$(pkg_version)_$(platform)$(if $(arch),_$(arch),).dmg

package_dmg:
	$(info Creating $(pkg_file))
	-$(call rmfiles_cmd,$(pkg_file))
	$(MAKE) $(pkg_file)

$(pkg_file): make_bundle
	@$(call showtime_cmd)
	$(path_step)/make/tools/make_dmg.sh -i $(wildcard $(macos_bundle)/Contents/Resources/*.icns) -n $@ $(macos_bundle)
