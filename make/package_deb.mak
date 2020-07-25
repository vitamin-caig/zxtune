#debian-related files
pkg_debian_root = $(pkg_dir)/pkg_deb
pkg_debian = $(pkg_debian_root)/debian

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
pkg_file = $(pkg_dir)/$(pkg_name)_$(pkg_version)_$(arch_deb)$(distro).deb

package_deb:
	$(info Creating $(pkg_file))
	-$(call rmfiles_cmd,$(pkg_file))
	$(MAKE) $(pkg_file)

$(pkg_file): $(pkg_debian)/changelog $(pkg_debian)/compat $(pkg_debian)/control $(pkg_debian)/docs $(pkg_debian)/rules $(pkg_debian)/copyright
	@$(call showtime_cmd)
	(cd $(pkg_debian_root) && dpkg-buildpackage -b) && mv $(pkg_dir)/$(pkg_name)_$(root.version.index)_$(arch_deb).deb $@
	$(call rmdir_cmd,$(pkg_debian_root))

$(pkg_debian):
	$(call makedir_cmd,$@)

$(pkg_debian)/changelog: $(dirs.root)/apps/changelog.txt | $(pkg_debian)
	$(dirs.root)/make/build/debian/convlog.pl <$^ $(pkg_name) $(root.version.index) > $@

$(pkg_debian)/compat: | $(pkg_debian)
	echo 7 > $@

$(pkg_debian)/control: dist/debian/control | $(pkg_debian)
	$(call copyfile_cmd,$^,$@);

escaped_curdir = $(shell echo $(CURDIR) | sed -r 's/\//\\\//g')

$(pkg_debian)/rules: $(dirs.root)/make/build/debian/rules | $(pkg_debian)
	sed -r 's/\$$\(target\)/platform=$(platform) arch=$(arch) distro=$(distro) -C $(escaped_curdir)/g' $^ > $@

$(pkg_debian)/copyright: $(dirs.root)/apps/copyright | $(pkg_debian)
	$(call copyfile_cmd,$^,$@)

$(pkg_debian)/docs: | $(pkg_debian)
	sed -r 's/.*/$(escaped_curdir)\/&/' dist/debian/docs > $@ || touch $@
