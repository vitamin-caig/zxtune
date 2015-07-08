binary_name := zxtune123
path_step := ../..
include $(path_step)/make/default.mak

source_files := $(addsuffix .cpp,information sound source display cli_app config)
source_files.windows = console_windows.cpp zxtune123.rc
source_files.mingw = $(source_files.windows)
source_files.linux = console_linux.cpp
source_files.macos = $(source_files.linux)
source_files.dingux = console_dingux.cpp

text_files := text/text
other_text_files := text/configfile text/cmdline text/messages text/conversion text/cmd_keys_local

libraries.common = analysis async binary binary_format \
                  core core_conversion core_plugins_players core_plugins_archives core_plugins_containers core_properties \
                  debug devices_aym devices_beeper devices_dac devices_aym_dumper devices_fm devices_saa devices_z80 \
                  formats_archived formats_archived_multitrack formats_chiptune formats_packed formats_multitrack \
                  io \
                  l10n_stub \
                  parameters platform platform_application platform_version \
                  sound sound_backends strings \
                  tools
libraries.3rdparty = gme lhasa lzma sidplayfp snesspc unrar xmp z80ex zlib

#platform
windows_libraries = advapi32 ole32 oldnames user32
mingw_libraries = ole32
libraries.boost = thread program_options system filesystem

manual_format := txt
manual_file = $(binary_name)_$(pkg_lang).$(manual_format)

#platform-specific resources
embedded_files = $(l10n_archive)

include $(path_step)/makefile.mak
include $(path_step)/make/package.mak

install_linux: install_linux_binary dist/$(manual_file)
	install -m644 -D ../changelog.txt $(DESTDIR)/usr/share/zxtune/changelog.txt
	install -m644 -D dist/zxtune.conf $(DESTDIR)/usr/share/zxtune/zxtune.conf
	install -m644 -D dist/$(manual_file) $(DESTDIR)/usr/share/zxtune/$(manual_file)

install_linux_binary:
	install -D $(target) $(DESTDIR)/usr/bin/$(binary_name)

install_windows: install_mingw install_runtime

install_mingw: install_samples install_3rdparty
	$(call copyfile_cmd,$(target),$(DESTDIR))
	$(call copyfile_cmd,dist/$(manual_file),$(DESTDIR))
	$(call copyfile_cmd,dist/zxtune.conf,$(DESTDIR))
	$(call copyfile_cmd,../changelog.txt,$(DESTDIR))

dingux_dir = local/apps/$(binary_name)

install_dingux: dist/$(manual_file) install_samples $(pkg_root)/wrapper.sh $(pkg_root)/dmenu.cfg $(pkg_root)/gmenu.cfg
	install -D $(target) $(DESTDIR)/$(dingux_dir)/$(notdir $(target))
	install -m755 -D $(pkg_root)/wrapper.sh $(DESTDIR)/$(dingux_dir)/$(binary_name).sh
	install -m644 -D $(pkg_root)/dmenu.cfg $(DESTDIR)/$(dingux_dir)/dmenu.cfg
	install -m644 -D $(pkg_root)/gmenu.cfg $(DESTDIR)/local/gmenu2x/sections/applications/$(binary_name)
	install -m644 -D ../changelog.txt $(DESTDIR)/changelog.txt
	install -m644 -D dist/zxtune.conf $(DESTDIR)/$(dingux_dir)/zxtune.conf
	install -m644 -D dist/$(manual_file) $(DESTDIR)/$(dingux_dir)/$(manual_file)
	install -m644 -D dist/dingux/readme.txt $(DESTDIR)/readme.txt
	install -m644 -D dist/dingux/zxtune.png $(DESTDIR)/$(dingux_dir)/zxtune.png
	install -m644 -D ../zxtune-qt/res/theme_default/zxtune32.png $(DESTDIR)/$(dingux_dir)/zxtune32.png
	$(call rmfiles_cmd,$(pkg_root)/wrapper.sh $(pkg_root)/dmenu.cfg $(pkg_root)/gmenu.cfg)

$(pkg_root)/wrapper.sh:
	@echo -e "\
	#!/bin/sh\n\
	./$(notdir $(target)) --progress --analyzer --oss --file \x24*\n\
	" > $@

$(pkg_root)/dmenu.cfg:
	@echo -e "\
	MenuItem ZXTune123\n\
	{\n\
	  Icon = \"/usr/$(dingux_dir)/zxtune.png\"\n\
	  Name = \"ZXTune123\"\n\
	  Executable = \"./$(binary_name).sh\"\n\
	  WorkDir = \"/usr/$(dingux_dir)/\"\n\
	}\n\
	" > $@

$(pkg_root)/gmenu.cfg:
	@echo -e "\
	title=ZXTune123\n\
	icon=/usr/$(dingux_dir)/zxtune32.png\n\
	exec=/usr/$(dingux_dir)/$(binary_name).sh\n\
	clock=430\n\
	backlight=100\n\
	selectordir=/usr/$(dingux_dir)/\n\
	selectorbrowser=true\n\
	" > $@

include $(path_step)/samples/samples.mak
include $(path_step)/3rdparty/install.mak

ifdef tools.textator
dist/$(manual_file): dist/$(binary_name).txt
	$(tools.textator) --process --keys $(pkg_lang),$(manual_format) --asm --output $@ $<
endif
