binary_name := zxtune123
dirs.root := ../..
include $(dirs.root)/make/default.mak

source_files := $(addsuffix .cpp,information sound source display cli_app config)
source_files.windows = console_windows.cpp zxtune123.rc
source_files.mingw = $(source_files.windows)
source_files.linux = console_linux.cpp
source_files.darwin = $(source_files.linux)

libraries.common = analysis async \
                  binary binary_compression binary_format \
                  core core_plugins_players core_plugins_archives \
                  debug devices_aym devices_beeper devices_dac devices_aym_dumper devices_fm devices_saa devices_z80 \
                  formats_archived formats_archived_multitrack formats_chiptune formats_packed formats_multitrack \
                  io \
                  l10n_stub \
                  module module_conversion module_players module_properties \
                  parameters platform platform_application platform_version \
                  sound sound_backends strings \
                  tools
libraries.3rdparty = asap atrac9 ffmpeg FLAC gme he ht hvl lazyusf2 lhasa lzma mgba mpg123 ogg openmpt sidplayfp snesspc sseqplayer unrar v2m vio2sf vgm vgmstream vorbis xmp z80ex zlib

#platform
libraries.windows = advapi32 ole32 oldnames shell32 user32
libraries.mingw = ole32
libraries.boost = filesystem program_options system

manual_format := txt
manual_file = $(binary_name)_$(pkg_lang).$(manual_format)

#platform-specific resources
embedded_files = $(l10n_archive)

include $(dirs.root)/makefile.mak
include $(dirs.root)/make/package.mak

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

include $(dirs.root)/samples/samples.mak
include $(dirs.root)/3rdparty/install.mak
