binary_name := xtractor
path_step := ../..
source_files := main

text_files := text/text

libraries := analysis async binary formats io tools zxtune_app_version lhasa z
depends := src/analysis src/async src/binary src/formats src/io src/tools apps/version 3rdparty/lhasa/lib 3rdparty/zlib

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
