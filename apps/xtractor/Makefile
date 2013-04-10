binary_name := xtractor
path_step := ../..
source_files := main

text_files := text/text

libraries := analysis async binary debug formats_archived formats_packed io l10n_stub platform strings tools zxtune_app_version
depends := $(addprefix src/,analysis async binary debug formats/archived formats/packed io l10n/stub platform strings tools) apps/version

#3rdparty
libraries +=  lhasa z unrar
depends += 3rdparty/lhasa/lib 3rdparty/zlib 3rdparty/unrar

boost_libraries := filesystem program_options thread system
windows_libraries := advapi32

#platform-specific resources
windows_resources := xtractor
mingw_resources := xtractor

include $(path_step)/makefile.mak
include $(path_step)/make/package.mak

install_linux: install_linux_binary

install_linux_binary:
	install -D $(target) $(DESTDIR)/usr/bin/$(binary_name)

install_windows install_mingw:
	$(call copyfile_cmd,$(target),$(DESTDIR))

dingux_dir = local/apps/$(binary_name)

install_dingux:
	install -D $(target) $(DESTDIR)/$(dingux_dir)/$(notdir $(target))
