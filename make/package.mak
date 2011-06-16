#package generating
pkg_revision := $(subst :,_,$(shell svnversion $(path_step)))
pkg_subversion := $(if $(release),,_dbg)

pkg_dir := $(path_step)/Builds/Revision$(pkg_revision)_$(platform)$(if $(arch),_$(arch),)
pkg_filename := $(binary_name)_r$(pkg_revision)$(pkg_subversion)_$(platform)_$(arch).$(pkg_suffix)
pkg_file := $(pkg_dir)/$(pkg_filename)
pkg_log := $(pkg_dir)/packaging_$(binary_name).log
pkg_build_log := $(pkg_dir)/$(binary_name).log
pkg_debug := $(pkg_dir)/$(binary_name)_debug.zip

temp_pkg = $(pkg_dir)/temp
debian_pkg = $(pkg_dir)/debian

package: | $(pkg_dir)
	@$(MAKE) -s clean_package
	$(info Creating package for $(binary_name) at $(pkg_dir))
	@$(MAKE) $(pkg_file) > $(pkg_log) 2>&1
	@$(call rmfiles_cmd,$(pkg_manual) $(pkg_build_log))

$(pkg_dir):
	$(call makedir_cmd,$@)

$(temp_pkg):
	$(call makedir_cmd,$@)

$(debian_pkg):
	$(call makedir_cmd,$@)

.PHONY: clean_package

clean_package:
	-$(call rmfiles_cmd,$(pkg_file) $(pkg_debug) $(pkg_build_log) $(pkg_log))
	-$(call rmdir_cmd,$(temp_pkg))

$(pkg_file): $(pkg_debug) $(pkg_additional_files) $(pkg_additional_files_$(platform)) | $(temp_pkg)
	@$(call showtime_cmd)
	$(info Packaging $(binary_name) to $(pkg_filename))
	@$(MAKE) DESTDIR=$(temp_pkg) install
	$(call makepkg_cmd,$(temp_pkg),$@)
	$(call rmdir_cmd,$(temp_pkg))
	-$(call rmdir_cmd,$(debian_pkg))

$(pkg_debug): $(pkg_build_log)
	@$(call showtime_cmd)
	$(info Packaging debug information and build log)
	zip -9Dj $@ $(target).pdb $(pkg_build_log)

$(pkg_build_log):
	@$(call showtime_cmd)
	$(info Building $(binary_name))
	$(MAKE) defines="ZXTUNE_VERSION=rev$(pkg_revision)" > $(pkg_build_log) 2>&1

#debian-related files
install_ubuntu: install_linux $(debian_pkg)/md5sums $(debian_pkg)/control
	install -m644 -D $(debian_pkg)/md5sums $(DESTDIR)/DEBIAN/md5sums
	install -m644 -D $(debian_pkg)/control $(DESTDIR)/DEBIAN/control

$(debian_pkg)/md5sums: $(debian_pkg)/copyright $(debian_pkg)/changelog | $(debian_pkg)
	install -m644 -D $(debian_pkg)/copyright $(temp_pkg)/usr/share/doc/$(binary_name)/copyright
	install -m644 -D $(debian_pkg)/changelog $(temp_pkg)/usr/share/doc/$(binary_name)/changelog
	gzip --best $(temp_pkg)/usr/share/doc/$(binary_name)/changelog
	md5sum `find $(temp_pkg) -type f` | sed 's| .*pkg\/| |' > $@

$(debian_pkg)/control: | $(debian_pkg)
	@echo "\
	Package: $(binary_name)\n\
	Version: $(pkg_revision)\n\
	Architecture: $(if $(arch),$(arch),`uname -m`)\n\
	Priority: optional\n\
	Section: sound\n\
	Maintainer: Vitamin <vitamin.caig@gmail.com>\n\
	Depends: libc6, libasound2\n\
	Description: The $(binary_name) application is used to play chiptunes from ZX Spectrum.\n\
	" > $@

$(debian_pkg)/copyright: | $(debian_pkg)
	@echo "\
	ZXTune\n\n\
	Copyright 2009-2011 Vitamin <vitamin.caig@gmail.com>\n\n\
	Visit http://zxtune.googlecode.com to get new versions and report about errors.\n\n\
	This software is distributed under GPLv3 license.\n\
	" > $@

$(debian_pkg)/changelog: | $(debian_pkg)
	@echo \
	"$(binary_name) (r$(pkg_revision)) experimental; urgency=low\n\n"\
	"  * See svn log for details\n"\
	" -- Vitamin <vitamin.caig@gmail.com>  "`date -R` > $@
