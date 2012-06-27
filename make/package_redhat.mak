#archlinux-related rules
pkg_redhat = $(pkg_dir)/rpm

pkg_rpm = $(pkg_dir)/$(pkg_name)-r$(pkg_revision)$(pkg_subversion)-1.$(arch).rpm

package_redhat:
	@$-$(call rmfiles_cmd,$(pkg_rpm) $(pkg_log))
	@$(info Creating package $(pkg_rpm))
	@$(MAKE) $(pkg_rpm) > $(pkg_log) 2>&1

$(pkg_rpm): pkg_dependency
	@$(call showtime_cmd)
	@$(MAKE) DESTDIR=$(pkg_root) install_redhat
	(cd $(pkg_redhat) && rpmbuild -bb --target $(arch)-unknown-linux rpm.spec \
	-D'_topdir $(CURDIR)/$(pkg_redhat)' -D'pkg_revision $(pkg_revision)' \
	&& mv RPMS/$(arch)/`basename $(pkg_rpm)` ..)
	$(call rmdir_cmd,$(pkg_redhat))

$(pkg_redhat):
	$(call makedir_cmd,$@/BUILD)
	$(call makedir_cmd,$@/RPMS)

install_redhat: $(pkg_redhat)/rpm.spec $(pkg_redhat)/BUILD/files.list

$(pkg_redhat)/rpm.spec: dist/rpm/spec | $(pkg_redhat)
	$(call copyfile_cmd,$^,$@)
	echo -e "\n\
%install\n\
make DESTDIR=%{buildroot} release=1 install -C `pwd`\n\
\n\
%files -f files.list\n" >> $@

$(pkg_redhat)/BUILD/files.list: install_linux | $(pkg_redhat)
	find $(CURDIR)/$(pkg_root)/usr -type f | sed -r 's/.*(\/usr.*)/\1/' > $@
