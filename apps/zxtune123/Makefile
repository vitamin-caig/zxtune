binary_name := zxtune123
path_step := ../..
include $(path_step)/make/default.mak

source_files := information sound source display cli_app
ifneq ($(findstring $(platform),windows mingw),)
source_files += console_windows
else ifneq ($(findstring $(platform),linux macos ubuntu archlinux),)
source_files += console_linux
else
source_files += console_$(platform)
endif

text_files := text/text
other_text_files := text/cmdline text/messages text/conversion text/cmd_keys_local

libraries := async core devices formats sound tools io zxtune_app_base
depends := src/async src/tools src/core src/devices src/formats src/sound src/io apps/base

windows_libraries = advapi32
boost_libraries = thread program_options

manual_format := txt
manual_file = $(binary_name)_$(pkg_lang).$(manual_format)

pkg_additional_files_dingux := $(wildcard dist/dingux/dmenu/*) $(wildcard dist/dingux/gmenu/*) dist/dingux/readme.txt dist/dingux/zxtune.png

include $(path_step)/makefile.mak
include $(path_step)/make/package.mak

install_linux: dist/$(manual_file)
	install -D $(target) $(DESTDIR)/usr/bin/$(binary_name)
	install -m644 -D ../zxtune.conf $(DESTDIR)/usr/share/zxtune/zxtune.conf
	install -m644 -D dist/$(manual_file) $(DESTDIR)/usr/share/zxtune/$(manual_file)

install_windows install_mingw: dist/$(manual_file) install_samples
	$(call copyfile_cmd,$(target),$(DESTDIR))
	$(call copyfile_cmd,dist/$(manual_file),$(DESTDIR))
	$(call copyfile_cmd,../zxtune.conf,$(DESTDIR))

install_dingux: dist/$(manual_file) install_samples
	install -D $(target) $(DESTDIR)/local/apps/zxtune123/$(binary_name)
	install -m644 -D ../zxtune.conf $(DESTDIR)/local/apps/zxtune123/zxtune.conf
	install -m644 -D dist/$(manual_file) $(DESTDIR)/local/apps/zxtune123/$(manual_file)
	install -m644 -D dist/dingux/dmenu/zxtune123.cfg $(DESTDIR)/local/apps/zxtune123/zxtune123.cfg
	install -m644 -D dist/dingux/gmenu/zxtune123 $(DESTDIR)/local/gmenu2x/sections/applications/zxtune123
	install -m644 -D dist/dingux/zxtune.png $(DESTDIR)/local/apps/zxtune123/zxtune.png
	install -m644 -D dist/dingux/readme.txt $(DESTDIR)/readme.txt

include $(path_step)/samples/samples.mak

dist/$(manual_file): dist/$(binary_name).txt
	$(TEXTATOR) --process --keys $(pkg_lang),$(manual_format) --asm --output $@ $<
