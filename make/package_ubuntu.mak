#debian-related files
pkg_debian = $(pkg_dir)/debian
ifneq ($(findstring $(arch),i386 i486 i586 i686),)
arch_deb = i386
else ifneq ($(findstring $(arch),x86_64 amd64),)
arch_deb = amd64
else ifneq ($(findstring $(arch),ppc powerpc),)
arch_deb = ppc
else
$(warning Unknown debian package architecture)
arch_deb = $(arch)
endif
pkg_deb = $(pkg_dir)/$(pkg_name)_r$(pkg_revision)$(pkg_subversion)_$(distro)_$(arch_deb).deb

package_ubuntu:
	@$-$(call rmfiles_cmd,$(pkg_deb) $(pkg_log))
	@$(info Creating package $(pkg_deb))
	@$(MAKE) $(pkg_deb) > $(pkg_log) 2>&1

$(pkg_deb): pkg_dependency
	@$(call showtime_cmd)
	@$(MAKE) DESTDIR=$(pkg_root) install_ubuntu
	fakeroot dpkg-deb --build $(pkg_root) $(CURDIR)/$@
	$(call rmdir_cmd,$(pkg_debian))

$(pkg_debian):
	$(call makedir_cmd,$@)

install_ubuntu: install_linux $(pkg_debian)/md5sums $(pkg_debian)/control
	install -m644 -D $(pkg_debian)/md5sums $(DESTDIR)/DEBIAN/md5sums
	install -m644 -D $(pkg_debian)/control $(DESTDIR)/DEBIAN/control

$(pkg_debian)/md5sums: $(pkg_debian)/copyright $(pkg_debian)/changelog | $(pkg_debian)
	install -m644 -D $(pkg_debian)/copyright $(pkg_root)/usr/share/doc/$(pkg_name)/copyright
	install -m644 -D $(pkg_debian)/changelog $(pkg_root)/usr/share/doc/$(pkg_name)/changelog
	gzip --best $(pkg_root)/usr/share/doc/$(pkg_name)/changelog
	md5sum `find $(pkg_root) -type f` | sed 's| .*$(pkg_root)\/| |' > $@

$(pkg_debian)/control: | $(pkg_debian)
	@echo -e "\
	Package: $(pkg_name)\n\
	Version: $(pkg_revision)\n\
	Architecture: $(arch_deb)\n\
	Priority: optional\n\
	Section: sound\n\
	Maintainer: Vitamin <vitamin.caig@gmail.com>\n\
	Depends: libc6, libasound2, zlib1g\n\
	Description: $(pkg_desc)\n\
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
	"$(pkg_name) (r$(pkg_revision)) experimental; urgency=low\n\n"\
	"  * See svn log for details\n"\
	" -- Vitamin <vitamin.caig@gmail.com>  "`date -R` > $@
