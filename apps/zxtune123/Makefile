binary_name := zxtune123
path_step := ../..
include $(path_step)/make/default.mak

source_files := information sound source display cli_app
ifneq ($(findstring $(platform),windows mingw),)
source_files += console_windows
else ifneq ($(findstring $(platform),linux macos),)
source_files += console_linux
else
source_files += console_$(platform)
endif

text_files := text/text
other_text_files := text/cmdline text/messages text/conversion text/cmd_keys_local

libraries := analysis async binary core devices formats sound tools io zxtune_app_base zxtune_app_version z80ex
depends := src/analysis src/async src/binary src/tools src/core src/devices src/formats src/sound src/io apps/base apps/version 3rdparty/z80ex

windows_libraries = advapi32
boost_libraries = thread program_options

manual_format := txt
manual_file = $(binary_name)_$(pkg_lang).$(manual_format)

#platform-specific resources
windows_resources := zxtune123
mingw_resources := zxtune123

include $(path_step)/makefile.mak
include $(path_step)/make/package.mak

install_linux: dist/$(manual_file)
	install -D $(target) $(DESTDIR)/usr/bin/$(binary_name)
	install -m644 -D ../zxtune.conf $(DESTDIR)/usr/share/zxtune/zxtune.conf
	install -m644 -D ../changelog.txt $(DESTDIR)/usr/share/zxtune/changelog.txt
	install -m644 -D dist/$(manual_file) $(DESTDIR)/usr/share/zxtune/$(manual_file)

install_windows install_mingw: dist/$(manual_file) install_samples
	$(call copyfile_cmd,$(target),$(DESTDIR))
	$(call copyfile_cmd,dist/$(manual_file),$(DESTDIR))
	$(call copyfile_cmd,../zxtune.conf,$(DESTDIR))
	$(call copyfile_cmd,../changelog.txt,$(DESTDIR))
	$(call copyfile_cmd,$(path_step)/3rdparty/lame/$(arch)/libmp3lame.dll,$(DESTDIR)/mp3lame.dll)
	$(call copyfile_cmd,$(path_step)/3rdparty/FLAC/$(arch)/FLAC.dll,$(DESTDIR))

dingux_dir = local/apps/$(binary_name)

install_dingux: dist/$(manual_file) install_samples $(pkg_root)/wrapper.sh $(pkg_root)/dmenu.cfg $(pkg_root)/gmenu.cfg
	install -D $(target) $(DESTDIR)/$(dingux_dir)/$(notdir $(target))
	install -m755 -D $(pkg_root)/wrapper.sh $(DESTDIR)/$(dingux_dir)/$(binary_name).sh
	install -m644 -D $(pkg_root)/dmenu.cfg $(DESTDIR)/$(dingux_dir)/dmenu.cfg
	install -m644 -D $(pkg_root)/gmenu.cfg $(DESTDIR)/local/gmenu2x/sections/applications/$(binary_name)
	install -m644 -D ../zxtune.conf $(DESTDIR)/$(dingux_dir)/zxtune.conf
	install -m644 -D ../changelog.txt $(DESTDIR)/changelog.txt
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

dist/$(manual_file): dist/$(binary_name).txt
	$(TEXTATOR) --process --keys $(pkg_lang),$(manual_format) --asm --output $@ $<
