pkg_file = $(pkg_dir)/$(pkg_name)_r$(pkg_revision)$(pkg_subversion)_rpi_armhf.zip
pkg_root = $(pkg_dir)/root

package_rpi:
	$(info Creating $(pkg_file))
	-$(call rmfiles_cmd,$(pkg_file))
	$(MAKE) $(pkg_file)

$(pkg_file): | $(pkg_root)
	@$(call showtime_cmd)
	$(MAKE) DESTDIR=$(pkg_root) install
	(cd $(pkg_root) && $(tools.root)zip -9rD $(CURDIR)/$@ .)
	$(call rmdir_cmd,$(pkg_root))

$(pkg_root):
	$(call makedir_cmd,$@)
