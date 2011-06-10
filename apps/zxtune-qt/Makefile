binary_name := zxtune-qt
path_step := ../..
include $(path_step)/make/default.mak

source_files = qt_app

#ui-related
gui_theme ?= restorer

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

#packaging
pkg_additional_files_dingux := $(wildcard dist/dingux/dmenu/*) $(wildcard dist/dingux/gmenu/*) $(wildcard dist/dingux/fonts/*) \
                               dist/dingux/zxtune-wrapper.sh dist/dingux/readme.txt $(theme_resources)/zxtune32.png

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

include $(path_step)/samples/samples.mak
