#archlinux-related rules
pkg_archlinux = $(pkg_dir)/archlinux

pkg_xz = $(pkg_dir)/$(pkg_name)-r$(pkg_revision)$(pkg_subversion)-1-$(arch).pkg.tar.xz

package_archlinux:
	@$-$(call rmfiles_cmd,$(pkg_xz) $(pkg_log))
	@$(info Creating package $(pkg_xz))
	@$(MAKE) $(pkg_xz) > $(pkg_log) 2>&1

$(pkg_xz): pkg_dependency pkg_files
	@$(call showtime_cmd)
	(cd $(pkg_dir) && makepkg -c -p archlinux/PKGBUILD --config archlinux/makepkg.conf)
	$(call rmdir_cmd,$(pkg_archlinux))

$(pkg_archlinux):
	$(call makedir_cmd,$@)

pkg_files: $(pkg_archlinux)/PKGBUILD $(pkg_archlinux)/makepkg.conf

$(pkg_archlinux)/PKGBUILD: dist/arch/pkgbuild | $(pkg_archlinux)
	$(call copyfile_cmd,$^,$@)
	@echo -e "\
	pkgver=r$(pkg_revision)\n\
	pkgrel=1\n\
	arch=('$(arch)')\n\
	options=(!strip !docs !libtool !emptydirs !zipman makeflags)\n\n\
	package() {\n\
	make DESTDIR=\x24{pkgdir} release=$(release) platform=$(platform) arch=$(arch) distro=$(distro) install -C `pwd`\n\
	}\n\
	" >> $@

$(pkg_archlinux)/makepkg.conf: | $(pkg_archlinux)
	@echo -e "\
	CARCH=\"$(arch)\"\n\
	CHOST=\"$(arch)-unknown-linux-gnu\"\n\
	BUILDENV=(fakeroot)\n\
	PKGEXT='.pkg.tar.xz'\n\
	" > $@
