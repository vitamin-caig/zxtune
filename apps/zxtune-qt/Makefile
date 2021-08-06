binary_name := zxtune-qt
dirs.root := ../..
include $(dirs.root)/make/default.mak

source_dirs = . supp
includes.dirs += .

source_files.windows = res/zxtune-qt.rc
source_files.mingw = $(source_files.windows)

#ui-related
#gui_theme ?= default
gui_theme ?= restorer
have_gui = 1

#other
moc_files += supp/playback_supp singlemode

#base
libraries.common = analysis async \
                  binary binary_compression binary_format \
                  core core_plugins_players core_plugins_archives \
                  devices_aym devices_aym_dumper devices_beeper devices_dac devices_fm devices_saa devices_z80 \
                  formats_archived formats_archived_multitrack formats_chiptune formats_packed formats_multitrack \
                  io \
                  module module_conversion module_players module_properties \
                  parameters platform platform_application platform_version \
                  resource \
                  sound sound_backends strings \
                  tools
libraries.3rdparty = asap atrac9 ffmpeg FLAC gme he ht hvl lazyusf2 lhasa lzma mgba mpg123 ogg openmpt sidplayfp snesspc sseqplayer unrar v2m vgm vgmstream vio2sf vorbis xmp z80ex zlib

#ui
libraries += playlist_ui playlist_supp playlist_io ui_common
depends += $(addprefix apps/zxtune-qt/,playlist/supp playlist/ui playlist/io ui)

libraries.boost = filesystem system

libraries.qt = Gui Core Network Widgets

libraries.common += l10n debug
libraries += ui_desktop playlist_ui_desktop ui_conversion ui_preferences check_update
depends += $(addprefix apps/zxtune-qt/,ui/desktop playlist/ui/desktop ui/conversion ui/preferences update)
libraries.boost += locale
libraries.darwin += iconv
embedded_files = $(qm_translation_data) $(mo_translation_data)

theme_resources = res/theme_$(gui_theme)

#qt-based resources
qrc_files := $(theme_resources)/theme

defines += GUI_THEME=$(gui_theme)

include $(dirs.root)/makefile.mak
include $(dirs.root)/make/package.mak

install_linux: install_linux_binary
	install -m644 -D ../changelog.txt $(DESTDIR)/usr/share/zxtune/changelog.txt

install_linux_binary:
	install -D $(target) $(DESTDIR)/usr/bin/$(binary_name)
	install -m644 -D dist/linux/zxTune.desktop $(DESTDIR)/usr/share/applications/zxTune.desktop
	install -m644 -D $(theme_resources)/zxtune.png $(DESTDIR)/usr/share/icons/zxtune.png

install_windows: install_mingw install_runtime

install_mingw: install_samples install_samples_playlist install_3rdparty
	$(call copyfile_cmd,$(target),$(DESTDIR))
	$(call copyfile_cmd,../changelog.txt,$(DESTDIR))

macos_bundle = $(bins_dir)/$(binary_name).app

make_bundle:
	mkdir -p $(macos_bundle)/Contents/MacOS
	install $(target) $(macos_bundle)/Contents/MacOS/$(binary_name)
	sed 's/@VERSION@/$(root.version.index).0.0/' dist/macos/Info.plist > $(macos_bundle)/Contents/Info.plist
	$(dirs.root)/make/tools/mkicns.sh $(theme_resources)/zxtune1024.png $(macos_bundle)/Contents/Resources/zxtune

include $(dirs.root)/samples/samples.mak
include $(dirs.root)/3rdparty/install.mak
