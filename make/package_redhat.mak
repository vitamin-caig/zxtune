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
	(cd $(pkg_redhat) && rpmbuild -bb --target $(arch)-unknown-linux rpm.spec && mv RPMS/$(arch)/`basename $(pkg_rpm)` ..)
	$(call rmdir_cmd,$(pkg_redhat))

$(pkg_redhat):
	$(call makedir_cmd,$@/BUILD)
	$(call makedir_cmd,$@/RPMS)

install_redhat: $(pkg_redhat)/rpm.spec

$(pkg_redhat)/rpm.spec: install_linux | $(pkg_redhat)
	@echo -e "\
%define _topdir $(CURDIR)/$(pkg_redhat)\n\
\n\
Summary:            $(pkg_desc)\n\
Name:               $(pkg_name)\n\
Version:            r${pkg_revision}\n\
Release:            1\n\
License:            GPLv3\n\
Group:              Multimedia\n\
URL:                http://zxtune.googlecode.com\n\
Distribution:       zxtune\n\
Vendor:             vitamin.caig@gmail.com\n\
ExclusiveOS:        linux\n\
ExclusiveArch:      $(arch)\n\
\n\
%description\n\
This application is the part of crossplatform zxtune toolkit.\n\
\n\
\n\
%install\n\
make DESTDIR=%{buildroot} release=1 install -C `pwd`\n\
\n\
%files\n\
`find $(CURDIR)/$(pkg_root)/usr -type f | sed -r 's/.*(\/usr.*)/\1/'`\n\
	" > $@
