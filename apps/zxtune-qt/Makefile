binary_name := zxtune-qt
path_step := ../..
include $(path_step)/make/default.mak

ifeq ($(platform),dingux)
embedded := 1
endif

source_dirs = . supp
include_dirs = .

source_files.windows = res/zxtune-qt.rc
source_files.mingw = $(source_files.windows)

#ui-related
#gui_theme ?= default
gui_theme ?= restorer
have_gui = 1

#other
moc_files += supp/playback_supp

text_files := text/text

#base
libraries.common = analysis async binary binary_format \
                  core core_conversion core_plugins_players core_plugins_archives core_plugins_containers core_properties \
                  devices_aym devices_aym_dumper devices_beeper devices_dac devices_fm devices_midi devices_saa devices_z80 \
                  formats_archived formats_chiptune formats_packed \
                  io \
                  parameters platform platform_application platform_version \
                  resource \
                  sound sound_backends strings \
                  tools
libraries.3rdparty = lhasa mt32emu sidplayfp sidplayfp_builders_resid unrar xmp z80ex zlib

#ui
libraries += playlist_ui playlist_supp playlist_io ui_common
depends += $(addprefix apps/zxtune-qt/,playlist/supp playlist/ui playlist/io ui)

libraries.boost = thread system filesystem

libraries.qt = Gui Core

ifdef embedded
libraries += ui_embedded playlist_ui_embedded
depends += apps/zxtune-qt/ui/embedded apps/zxtune-qt/playlist/ui/embedded
libraries.qt += Network
else
libraries.common += l10n debug
libraries += ui_desktop playlist_ui_desktop ui_conversion ui_preferences check_update
depends += $(addprefix apps/zxtune-qt/,ui/desktop playlist/ui/desktop ui/conversion ui/preferences update)
libraries.boost += locale
mingw_libraries += iconv
embedded_files = $(qm_translation_data) $(mo_translation_data)
endif

theme_resources = res/theme_$(gui_theme)

#qt-based resources
qrc_files := $(theme_resources)/theme

$(platform)_definitions += GUI_THEME=$(gui_theme)

include $(path_step)/makefile.mak
include $(path_step)/make/package.mak

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
include $(path_step)/3rdparty/install.mak
