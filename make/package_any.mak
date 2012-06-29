pkg_file = $(pkg_dir)/$(pkg_name)_r$(pkg_revision)$(pkg_subversion)_$(platform)$(if $(arch),_$(arch),).$(pkg_suffix)
pkg_root = $(pkg_dir)/root

package_any:
	$(info Creating $(pkg_file))
	-$(call rmfiles_cmd,$(pkg_file))
	$(MAKE) $(pkg_file)

$(pkg_file): | $(pkg_root)
	@$(call showtime_cmd)
	$(MAKE) DESTDIR=$(pkg_root) install
	$(call makepkg_cmd,$(pkg_root),$@)
	$(call rmdir_cmd,$(pkg_root))

$(pkg_root):
	$(call makedir_cmd,$@)
