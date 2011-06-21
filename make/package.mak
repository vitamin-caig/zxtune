#package generating
pkg_revision := $(subst :,_,$(shell svnversion $(path_step)))
pkg_subversion := $(if $(release),,_dbg)

pkg_tag := $(platform)$(if $(arch),_$(arch),)$(if $(distro),_$(distro),)
pkg_dir := $(path_step)/Builds/Revision$(pkg_revision)_$(pkg_tag)
pkg_filename := $(binary_name)_r$(pkg_revision)$(pkg_subversion)_$(pkg_tag)
pkg_file := $(pkg_dir)/$(pkg_filename)
pkg_log := $(pkg_dir)/packaging_$(binary_name).log
pkg_build_log := $(pkg_dir)/$(binary_name).log
pkg_debug := $(pkg_dir)/$(binary_name)_debug.zip

pkg_root = $(pkg_dir)/root

package: package_$(distro)

package_any:
	@-$(call rmfile_cmd,$(pkg_file).$(pkg_suffix) $(pkg_log))
	@$(call makedir_cmd,$(pkg_dir))
	@$(MAKE) $(pkg_file).$(pkg_suffix) > $(pkg_log) 2>&1

$(pkg_file).$(pkg_suffix): $(pkg_debug)
	@$(call showtime_cmd)
	$(info Packaging $(binary_name) to $(pkg_suffix) package)
	$(call makedir_cmd,$(pkg_root))
	@$(MAKE) DESTDIR=$(pkg_root) install
	$(call makepkg_cmd,$(pkg_root),$@)
	$(call rmdir_cmd,$(pkg_root))

$(pkg_root):
	$(call makedir_cmd,$@)

$(pkg_debug): $(pkg_build_log)
	@$(call showtime_cmd)
	$(info Packaging debug information and build log)
	zip -9Dj $@ $(target).pdb
	zip -9Djm $@ $(pkg_build_log)

$(pkg_build_log):
	@$(call showtime_cmd)
	$(info Building $(binary_name))
	$(MAKE) defines="ZXTUNE_VERSION=rev$(pkg_revision)" > $(pkg_build_log) 2>&1

#debian-related files
pkg_debian = $(pkg_dir)/debian

package_ubuntu:
	$-$(call rmfile_cmd,$(pkg_file).deb $(pkg_log))
	@$(call makedir_cmd,$(pkg_dir))
	@$(MAKE) $(pkg_file).deb > $(pkg_log) 2>&1

$(pkg_file).deb: $(pkg_debug)
	@$(call showtime_cmd)
	$(info Packaging $(binary_name) to deb package)
	$(call makedir_cmd,$(pkg_root))
	@$(MAKE) DESTDIR=$(pkg_root) install_ubuntu
	fakeroot dpkg-deb --build $(pkg_root) $(CURDIR)/$@
	$(call rmdir_cmd,$(pkg_root))
	$(call rmdir_cmd,$(pkg_debian))

$(pkg_debian):
	$(call makedir_cmd,$@)

install_ubuntu: install_linux $(pkg_debian)/md5sums $(pkg_debian)/control
	install -m644 -D $(pkg_debian)/md5sums $(DESTDIR)/DEBIAN/md5sums
	install -m644 -D $(pkg_debian)/control $(DESTDIR)/DEBIAN/control

$(pkg_debian)/md5sums: $(pkg_debian)/copyright $(pkg_debian)/changelog | $(pkg_debian)
	install -m644 -D $(pkg_debian)/copyright $(pkg_root)/usr/share/doc/$(binary_name)/copyright
	install -m644 -D $(pkg_debian)/changelog $(pkg_root)/usr/share/doc/$(binary_name)/changelog
	gzip --best $(pkg_root)/usr/share/doc/$(binary_name)/changelog
	md5sum `find $(pkg_root) -type f` | sed 's| .*$(pkg_root)\/| |' > $@

ifneq ($(findstring $(arch),i386 i486 i586 i686),)
arch_debian := i386
else ifneq ($(findstring $(arch),x86_64),)
arch_debian := amd64
else
arch_debian := unknown
endif

$(pkg_debian)/control: | $(pkg_debian)
	@echo -e "\
	Package: $(binary_name)\n\
	Version: $(pkg_revision)\n\
	Architecture: $(arch_debian)\n\
	Priority: optional\n\
	Section: sound\n\
	Maintainer: Vitamin <vitamin.caig@gmail.com>\n\
	Depends: libc6, libasound2, zlib1g\n\
	Description: The $(binary_name) application is used to play chiptunes from ZX Spectrum.\n\
	" > $@

$(pkg_debian)/copyright: | $(pkg_debian)
	@echo -e "\
	ZXTune\n\n\
	Copyright 2009-2011 Vitamin <vitamin.caig@gmail.com>\n\n\
	Visit http://zxtune.googlecode.com to get new versions and report about errors.\n\n\
	This software is distributed under GPLv3 license.\n\
	" > $@

$(pkg_debian)/changelog: | $(pkg_debian)
	@echo -e \
	"$(binary_name) (r$(pkg_revision)) experimental; urgency=low\n\n"\
	"  * See svn log for details\n"\
	" -- Vitamin <vitamin.caig@gmail.com>  "`date -R` > $@

#archlinux-related rules
pkg_archlinux = $(pkg_dir)/archlinux

package_archlinux:
	$-$(call rmfile_cmd,$(pkg_file).deb $(pkg_log))
	@$(call makedir_cmd,$(pkg_dir))
	@$(MAKE) $(pkg_file).tar.xz > $(pkg_log) 2>&1

$(pkg_file).tar.xz: $(pkg_debug)
	@$(call showtime_cmd)
	$(info Packaging $(binary_name) to tar.xz package)
	$(call makedir_cmd,$(pkg_root))
	@$(MAKE) DESTDIR=$(pkg_root) install_archlinux
	(cd $(pkg_dir) && makepkg -c -p archlinux/PKGBUILD)
	$(call rmdir_cmd,$(pkg_root))
	$(call rmdir_cmd,$(pkg_archlinux))

$(pkg_archlinux):
	$(call makedir_cmd,$@)

install_archlinux: install_linux $(pkg_archlinux)/PKGBUILD

$(pkg_archlinux)/PKGBUILD: | $(pkg_archlinux)
	@echo -e "\
	# Maintainer: vitamin.caig@gmail.com\n\
	pkgname=$(binary_name)\n\
	pkgver=r$(pkg_revision)\n\
	pkgrel=1\n\
	pkgdesc=\"The $(binary_name) application is used to play chiptunes from ZX Spectrum\"\n\
	arch=('$(arch)')\n\
	url=\"http://zxtune.googlecode.com\"\n\
	license=('GPL3')\n\
	depends=(zlib >= 1.2.3)\n\
	provides=('$(binary_name)')\n\
	options=(!strip !docs !libtool !emptydirs !zipman makeflags)\n\n\
	package() {\n\
	cp -R $(CURDIR)/$(pkg_root)/* \x24{pkgdir}\n\
	}\n\
	" > $@
