binary_name := zxtune-qt
path_step := ../..
include $(path_step)/make/default.mak

source_files = qt_app

#ui-related
gui_theme ?= default

#other
moc_files += supp/playback_supp

include_dirs = .

text_files := text/text

libraries = zxtune_app_base async core devices formats io sound tools \
             playlist_ui playlist_supp playlist_io ui_common

depends = apps/base src/async src/core src/devices src/formats src/io src/sound src/tools \
           apps/zxtune-qt/playlist/supp apps/zxtune-qt/playlist/ui apps/zxtune-qt/playlist/io \
           apps/zxtune-qt/ui

boost_libraries = thread

qt_libraries = Gui Core

ifeq ($(platform),dingux)
embedded := 1
qt_libraries += Network
endif

ifdef embedded
libraries += ui_embedded
depends += apps/zxtune-qt/ui/embedded
else
libraries += ui_desktop
depends += apps/zxtune-qt/ui/desktop
endif

theme_resources = res/theme_$(gui_theme)

#qt-based resources
qrc_files := res/icons $(theme_resources)/theme

#platform-specific resources
windows_resources := res/zxtune-qt
mingw_resources := res/zxtune-qt

$(platform)_definitions += GUI_THEME=$(gui_theme)

include $(path_step)/makefile.mak
include $(path_step)/make/package.mak

install_linux:
	install -D $(target) $(DESTDIR)/usr/bin/$(binary_name)
	install -m644 -D ../zxtune.conf $(DESTDIR)/usr/share/zxtune/zxtune.conf
	install -m644 -D dist/linux/zxTune.desktop $(DESTDIR)/usr/share/applications/zxTune.desktop
	install -m644 -D $(theme_resources)/zxtune.png $(DESTDIR)/usr/share/icons/zxtune.png

install_windows install_mingw: install_samples install_samples_playlist
	$(call copyfile_cmd,$(target),$(DESTDIR))
	$(call copyfile_cmd,../zxtune.conf,$(DESTDIR))

install_dingux: dist/$(manual_file) install_samples install_samples_playlist
	install -D $(target) $(DESTDIR)/local/apps/zxtune/$(binary_name)
	install -m755 -D dist/dingux/zxtune-wrapper.sh $(DESTDIR)/local/apps/zxtune/zxtune-wrapper.sh
	install -m644 -D disk/dingux/fonts/fixed_70_50.qpf $(DESTDIR)/local/share/fonts/fixed_70_50.qpf
	install -m644 -D ../zxtune.conf $(DESTDIR)/local/apps/zxtune/zxtune.conf
	install -m644 -D dist/dingux/dmenu/zxtune.cfg $(DESTDIR)/local/apps/zxtune/zxtune.cfg
	install -m644 -D dist/dingux/gmenu/zxtune $(DESTDIR)/local/gmenu2x/sections/applications/zxtune
	install -m644 -D $(theme_resources)/zxtune32.png $(DESTDIR)/local/apps/zxtune/zxtune32.png
	install -m644 -D dist/dingux/readme.txt $(DESTDIR)/readme.txt

include $(path_step)/samples/samples.mak
