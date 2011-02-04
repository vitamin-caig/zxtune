binary_name := zxtune-qt
path_step := ../..
include $(path_step)/make/default.mak

source_files = qt_app \
               ui/format \
               ui/tools/filedialog

#ui-related

#main window is specific for platforms
ifeq ($(platform),dingux)
ui_files += ui/mainwindow_embedded
else
ui_files += ui/mainwindow
endif

#controls
ui_files += ui/controls/playback_controls \
            ui/controls/seek_controls \
            ui/controls/status_control \
            ui/controls/volume_control

moc_files += ui/controls/analyzer_control

#informational
ui_files += ui/informational/aboutdialog \
            ui/informational/componentsdialog

#other
moc_files += supp/playback_supp \
             ui/tools/parameters_helpers

include_dirs = .

text_files := text/text

libraries := zxtune_app_base core devices io sound tools \
             playlist_ui playlist_supp playlist_io
depends := apps/base src/core src/devices src/io src/sound src/tools \
           apps/zxtune-qt/playlist/supp apps/zxtune-qt/playlist/ui apps/zxtune-qt/playlist/io

boost_libraries = thread

qt_libraries = Gui Core
ifeq ($(platform),dingux)
qt_libraries += Network
endif

theme_resources = res/theme_$(gui_theme)

#packaging
pkg_additional_files := ../zxtune.conf
pkg_additional_files_linux := dist/linux/zxTune.desktop $(theme_resources)/zxtune.png
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

install:
	install -D $(target) $(DESTDIR)/usr/bin/$(binary_name)
	install -m644 -D ../zxtune.conf $(DESTDIR)/usr/share/zxtune/zxtune.conf
	install -m644 -D dist/linux/zxTune.desktop $(DESTDIR)/usr/share/applications/zxTune.desktop
	install -m644 -D $(theme_resources)/zxtune.png $(DESTDIR)/usr/share/icons/zxtune.png
