binary_name := zxtune-qt
path_step := ../..
include $(path_step)/make/default.mak

ifeq ($(platform),dingux)
embedded := 1
endif

source_files = qt_app supp/options

#ui-related
#gui_theme ?= default
gui_theme ?= restorer

#other
moc_files += supp/playback_supp supp/mixer

include_dirs = .

text_files := text/text

libraries = zxtune_app_base zxtune_app_version analysis async binary core devices formats io platform sound tools \
            playlist_ui playlist_supp playlist_io ui_common ui_conversion ui_preferences

depends = apps/base apps/version src/analysis src/async src/binary src/core src/devices src/formats src/io src/platform src/sound src/tools \
          apps/zxtune-qt/playlist/supp apps/zxtune-qt/playlist/ui apps/zxtune-qt/playlist/io \
          apps/zxtune-qt/ui apps/zxtune-qt/ui/conversion apps/zxtune-qt/ui/preferences

#3rdparty
libraries += lhasa z z80ex
depends += 3rdparty/lhasa/lib 3rdparty/z80ex 3rdparty/zlib

boost_libraries = thread system

qt_libraries = Gui Core

ifdef embedded
libraries += l10n_stub ui_embedded playlist_ui_embedded
depends += src/l10n/stub apps/zxtune-qt/ui/embedded apps/zxtune-qt/playlist/ui/embedded
qt_libraries += Network
else
libraries += l10n ui_desktop playlist_ui_desktop
depends += src/l10n apps/zxtune-qt/ui/desktop apps/zxtune-qt/playlist/ui/desktop
boost_libraries += locale
mingw_libraries += iconv
endif

theme_resources = res/theme_$(gui_theme)

#qt-based resources
qrc_files := res/icons $(theme_resources)/theme

#platform-specific resources
windows_resources := res/zxtune-qt
mingw_resources := res/zxtune-qt
embedded_files = $(qm_translation_data) $(mo_translation_data)

$(platform)_definitions += GUI_THEME=$(gui_theme)

include $(path_step)/makefile.mak
include $(path_step)/make/package.mak

install_linux: install_linux_binary
	install -m644 -D ../changelog.txt $(DESTDIR)/usr/share/zxtune/changelog.txt

install_linux_binary:
	install -D $(target) $(DESTDIR)/usr/bin/$(binary_name)
	install -m644 -D dist/linux/zxTune.desktop $(DESTDIR)/usr/share/applications/zxTune.desktop
	install -m644 -D $(theme_resources)/zxtune.png $(DESTDIR)/usr/share/icons/zxtune.png

install_windows install_mingw: install_samples install_samples_playlist
	$(call copyfile_cmd,$(target),$(DESTDIR))
	$(call copyfile_cmd,../changelog.txt,$(DESTDIR))
	$(call copyfile_cmd,$(path_step)/3rdparty/lame/$(arch)/libmp3lame.dll,$(DESTDIR)/mp3lame.dll)
	$(call copyfile_cmd,$(path_step)/3rdparty/FLAC/$(arch)/FLAC.dll,$(DESTDIR))
	$(call copyfile_cmd,$(path_step)/3rdparty/ogg/$(arch)/ogg.dll,$(DESTDIR))
	$(call copyfile_cmd,$(path_step)/3rdparty/vorbis/$(arch)/*.dll,$(DESTDIR))
	$(call copyfile_cmd,$(path_step)/3rdparty/curl/$(arch)/*.dll,$(DESTDIR))

dingux_dir = local/apps/$(binary_name)

install_dingux: dist/$(manual_file) install_samples install_samples_playlist $(pkg_root)/wrapper.sh $(pkg_root)/dmenu.cfg $(pkg_root)/gmenu.cfg
	install -D $(target) $(DESTDIR)/$(dingux_dir)/$(notdir $(target))
	install -m755 -D $(pkg_root)/wrapper.sh $(DESTDIR)/$(dingux_dir)/$(binary_name).sh
	install -m644 -D $(pkg_root)/dmenu.cfg $(DESTDIR)/$(dingux_dir)/dmenu.cfg
	install -m644 -D $(pkg_root)/gmenu.cfg $(DESTDIR)/local/gmenu2x/sections/applications/$(binary_name)
	install -m644 -D dist/dingux/fonts/fixed_70_50.qpf $(DESTDIR)/local/share/fonts/fixed_70_50.qpf
	install -m644 -D ../changelog.txt $(DESTDIR)/changelog.txt
	install -m644 -D $(theme_resources)/zxtune32.png $(DESTDIR)/$(dingux_dir)/zxtune32.png
	install -m644 -D dist/dingux/readme.txt $(DESTDIR)/readme.txt
	$(call rmfiles_cmd,$(pkg_root)/wrapper.sh $(pkg_root)/dmenu.cfg $(pkg_root)/gmenu.cfg)

$(pkg_root)/wrapper.sh:
	@echo -e "\
	#!/bin/sh\n\
	QT_QWS_FONTDIR=/usr/local/share/fonts QWS_KEYBOARD=USB:/dev/event0 ./$(notdir $(target)) -qws -nomouse\n\
	" > $@

$(pkg_root)/dmenu.cfg:
	@echo -e "\
	MenuItem ZXTuneQT\n\
	{\n\
	  Icon = \"/usr/$(dingux_dir)/zxtune32.png\"\n\
	  Name = \"ZXTuneQT\"\n\
	  Executable = \"./$(binary_name).sh\"\n\
	  WorkDir = \"/usr/$(dingux_dir)/\"\n\
	}\n\
	" > $@

$(pkg_root)/gmenu.cfg:
	@echo -e "\
	title=ZXTuneQT\n\
	icon=/usr/$(dingux_dir)/zxtune32.png\n\
	exec=/usr/$(dingux_dir)/$(binary_name).sh\n\
	clock=430\n\
	backlight=100\n\
	" > $@

include $(path_step)/samples/samples.mak
