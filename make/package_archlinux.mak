#archlinux-related rules
pkg_archlinux = $(pkg_dir)/archlinux
#pkgver cannot contain hyphens or etc
pkg_version_arch = $(firstword $(subst -, ,$(pkg_version)))
pkg_file = $(pkg_dir)/$(pkg_name)-$(pkg_version)-1-$(arch).pkg.tar.xz

package_archlinux:
	$(info Creating package $(pkg_file))
	-$(call rmfiles_cmd,$(pkg_file) $(pkg_log))
	$(MAKE) $(pkg_file) > $(pkg_log) 2>&1

$(pkg_file): $(pkg_archlinux)/PKGBUILD $(pkg_archlinux)/makepkg.conf
	@$(call showtime_cmd)
	(cd $(pkg_archlinux) && makepkg -c -p PKGBUILD --config makepkg.conf && mv *.pkg.tar.xz ..)
	$(call rmdir_cmd,$(pkg_archlinux))

$(pkg_archlinux):
	$(call makedir_cmd,$@)

$(pkg_archlinux)/PKGBUILD: dist/arch/pkgbuild | $(pkg_archlinux)
	$(call copyfile_cmd,$^,$@)
	@echo -e "\
	pkgver=$(pkg_version_arch)\n\
	pkgrel=1\n\
	arch=('$(arch)')\n\
	options=(!strip !docs !libtool !emptydirs !zipman makeflags)\n\n\
	package() {\n\
	make DESTDIR=\x24{pkgdir} platform=$(platform) arch=$(arch) distro=$(distro) install -C `pwd`\n\
	}\n\
	" >> $@

$(pkg_archlinux)/makepkg.conf: | $(pkg_archlinux)
	@echo -e "\
	CARCH=\"$(arch)\"\n\
	CHOST=\"$(arch)-unknown-linux-gnu\"\n\
	BUILDENV=(fakeroot)\n\
	PKGEXT='.pkg.tar.xz'\n\
	" > $@
