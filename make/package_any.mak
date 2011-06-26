pkg_any = $(pkg_dir)/$(binary_name)_r$(pkg_revision)$(pkg_subversion)_$(pkg_tag).$(pkg_suffix)

package_any:
	@-$(call rmfiles_cmd,$(pkg_any) $(pkg_log))
	@$(info Creating package $(pkg_any))
	@$(MAKE) $(pkg_any) > $(pkg_log) 2>&1

$(pkg_any): pkg_dependency
	@$(call showtime_cmd)
	@$(MAKE) DESTDIR=$(pkg_root) install
	@$(call makepkg_cmd,$(pkg_root),$@)
