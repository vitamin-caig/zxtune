#debian-related files
pkg_debian_root = $(pkg_dir)/$(pkg_name)_$(pkg_revision)$(pkg_subversion)
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
pkg_deb = $(pkg_dir)/$(pkg_name)_$(pkg_revision)$(pkg_subversion)_$(arch_deb).deb

package_ubuntu:
	@$-$(call rmfiles_cmd,$(pkg_deb) $(pkg_log))
	@$(info Creating package $(pkg_deb))
	@$(MAKE) $(pkg_deb) > $(pkg_log) 2>&1

$(pkg_deb): pkg_dependency dpkg_files
	@$(call showtime_cmd)
	(cd $(pkg_debian_root) && dpkg-buildpackage -b )
	$(call rmdir_cmd,$(pkg_debian_root))

$(pkg_debian):
	$(call makedir_cmd,$@)

dpkg_files: $(pkg_debian)/changelog $(pkg_debian)/compat $(pkg_debian)/control $(pkg_debian)/docs $(pkg_debian)/rules $(pkg_debian)/copyright

$(pkg_debian)/changelog: $(path_step)/apps/changelog.txt | $(pkg_debian)
	$(path_step)/make/convlog.pl <$^ $(pkg_name) $(pkg_revision)$(pkg_subversion) > $@

$(pkg_debian)/compat: | $(pkg_debian)
	@echo 7 > $@

$(pkg_debian)/control: dist/debian/control | $(pkg_debian)
	$(call copyfile_cmd,$^,$@);

escaped_curdir = $(shell echo $(CURDIR) | sed -r 's/\//\\\//g')

$(pkg_debian)/rules: $(path_step)/make/build/debian/rules | $(pkg_debian)
	@sed -r 's/\$$\(target\)/release=$(release) platform=$(platform) arch=$(arch) distro=$(distro) -C $(escaped_curdir)/g' $^ > $@

$(pkg_debian)/copyright: $(path_step)/apps/copyright | $(pkg_debian)
	$(call copyfile_cmd,$^,$@)

$(pkg_debian)/docs: | $(pkg_debian)
	@sed -r 's/.*/$(escaped_curdir)\/&/' dist/debian/docs > $@ || touch $@
