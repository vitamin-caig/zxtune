#package generating
pkg_revision := $(subst :,_,$(shell svnversion $(path_step)))
pkg_subversion := $(if $(release),,_dbg)

pkg_dir := $(path_step)/Builds/Revision$(pkg_revision)_$(platform)$(if $(arch),_$(arch),)
pkg_filename := $(binary_name)_r$(pkg_revision)$(pkg_subversion)_$(platform)_$(arch).$(pkg_suffix)
pkg_file := $(pkg_dir)/$(pkg_filename)
pkg_log := $(pkg_dir)/packaging_$(binary_name).log
pkg_build_log := $(pkg_dir)/$(binary_name).log
pkg_debug := $(pkg_dir)/$(binary_name)_debug.zip

pkg_root = $(pkg_dir)/root
pkg_debian = $(pkg_dir)/debian
pkg_archlinux = $(pkg_dir)/archlinux

package: | $(pkg_dir)
	@$(MAKE) -s clean_package
	$(info Creating package for $(binary_name) at $(pkg_dir))
	@$(MAKE) $(pkg_file) > $(pkg_log) 2>&1
	@$(call rmfiles_cmd,$(pkg_manual) $(pkg_build_log))

$(pkg_dir):
	$(call makedir_cmd,$@)

$(pkg_root):
	$(call makedir_cmd,$@)

$(pkg_debian):
	$(call makedir_cmd,$@)

$(pkg_archlinux):
	$(call makedir_cmd,$@)

.PHONY: clean_package

clean_package:
	-$(call rmfiles_cmd,$(pkg_file) $(pkg_debug) $(pkg_build_log) $(pkg_log))
	-$(call rmdir_cmd,$(pkg_root))

$(pkg_file): $(pkg_debug) $(pkg_additional_files) $(pkg_additional_files_$(platform)) | $(pkg_root)
	@$(call showtime_cmd)
	$(info Packaging $(binary_name) to $(pkg_filename))
	@$(MAKE) DESTDIR=$(pkg_root) install
	$(call makepkg_cmd,$(pkg_root),$@)
	$(call rmdir_cmd,$(pkg_root))
	-$(call rmdir_cmd,$(pkg_debian))
	-$(call rmdir_cmd,$(pkg_archlinux))

$(pkg_debug): $(pkg_build_log)
	@$(call showtime_cmd)
	$(info Packaging debug information and build log)
	zip -9Dj $@ $(target).pdb $(pkg_build_log)

$(pkg_build_log):
	@$(call showtime_cmd)
	$(info Building $(binary_name))
	$(MAKE) defines="ZXTUNE_VERSION=rev$(pkg_revision)" > $(pkg_build_log) 2>&1

#debian-related files
install_ubuntu: install_linux $(pkg_debian)/md5sums $(pkg_debian)/control
	install -m644 -D $(pkg_debian)/md5sums $(DESTDIR)/DEBIAN/md5sums
	install -m644 -D $(pkg_debian)/control $(DESTDIR)/DEBIAN/control

$(pkg_debian)/md5sums: $(pkg_debian)/copyright $(pkg_debian)/changelog | $(pkg_debian)
	install -m644 -D $(pkg_debian)/copyright $(pkg_root)/usr/share/doc/$(binary_name)/copyright
	install -m644 -D $(pkg_debian)/changelog $(pkg_root)/usr/share/doc/$(binary_name)/changelog
	gzip --best $(pkg_root)/usr/share/doc/$(binary_name)/changelog
	md5sum `find $(pkg_root) -type f` | sed 's| .*$(pkg_root)\/| |' > $@

$(pkg_debian)/control: | $(pkg_debian)
	@echo -e "\
	Package: $(binary_name)\n\
	Version: $(pkg_revision)\n\
	Architecture: $(if $(arch),$(arch),`uname -m`)\n\
	Priority: optional\n\
	Section: sound\n\
	Maintainer: Vitamin <vitamin.caig@gmail.com>\n\
	Depends: libc6, libasound2\n\
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
install_archlinux: install_linux $(pkg_archlinux)/PKGBUILD

$(pkg_archlinux)/PKGBUILD: | $(pkg_archlinux)
	@echo -e "\
	# Maintainer: vitamin.caig@gmail.com\n\
	pkgname=$(binary_name)\n\
	pkgver=r$(pkg_revision)\n\
	pkgrel=1\n\
	pkgdesc=\"The $(binary_name) application is used to play chiptunes from ZX Spectrum\"\n\
	arch=('$(if $(arch),$(arch),`uname -m`)')\n\
	url=\"http://zxtune.googlecode.com\"\n\
	license=('GPL3')\n\
	depends=()\n\
	provides=('$(binary_name)')\n\
	options=(!strip !docs !libtool !emptydirs !zipman makeflags)\n\n\
	package() {\n\
	cp -R $(CURDIR)/$(pkg_root)/* \x24{pkgdir}\n\
	}\n\
	" > $@
