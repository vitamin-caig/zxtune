#archlinux-related rules
pkg_rpm = $(pkg_dir)/rpm
pkg_root = $(pkg_dir)/root
pkg_file = $(pkg_dir)/$(pkg_name)-r$(pkg_revision)$(pkg_subversion)-1.$(if $(distro),$(distro).,)$(arch).rpm

package_rpm:
	$(info Creating $(pkg_file))
	-$(call rmfiles_cmd,$(pkg_file))
	$(MAKE) $(pkg_file) > $(pkg_log) 2>&1

$(pkg_file): $(pkg_rpm)/rpm.spec $(pkg_rpm)/BUILD/files.list
	@$(call showtime_cmd)
	(cd $(pkg_rpm) && rpmbuild -bb --target $(arch)-unknown-linux rpm.spec \
	-D'_topdir $(CURDIR)/$(pkg_rpm)' -D'pkg_revision $(pkg_revision)' ) && \
	mv $(pkg_rpm)/RPMS/$(arch)/*.rpm $@
	$(call rmdir_cmd,$(pkg_rpm))

$(pkg_rpm):
	$(call makedir_cmd,$@/BUILD)
	$(call makedir_cmd,$@/RPMS)

$(pkg_root):
	$(call makedir_cmd,$@)

$(pkg_rpm)/rpm.spec: dist/rpm/spec | $(pkg_rpm)
	$(call copyfile_cmd,$^,$@)
	echo -e "\n\
%install\n\
make DESTDIR=%{buildroot} release=$(release) platform=$(platform) arch=$(arch) distro=$(distro) install -C `pwd`\n\
\n\
%files -f files.list\n" >> $@


$(pkg_rpm)/BUILD/files.list: | $(pkg_rpm) $(pkg_root)
	$(MAKE) DESTDIR=$(pkg_root) install_linux
	find $(CURDIR)/$(pkg_root) -type f | sed -r 's/.*(\/usr.*)/\1/' > $@
	$(call rmdir_cmd,$(pkg_root))