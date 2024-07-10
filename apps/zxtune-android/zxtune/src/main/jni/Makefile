dynamic_name := zxtune
dirs.root := ../../../../../..

source_dirs := .

libraries.common = analysis \
                   binary binary_compression binary_format \
                   core core_plugins_archives_lite core_plugins_players \
                   devices_aym devices_beeper devices_dac devices_fm devices_saa devices_z80 \
                   formats_chiptune formats_packed_archives formats_packed_decompilers formats_archived formats_archived_multitrack formats_multitrack \
                   module module_players \
                   parameters \
                   sound strings \
                   tools

libraries.3rdparty = asap atrac9 ffmpeg FLAC gme he ht hvl lazyusf2 lhasa lzma mpg123 ogg openmpt opus mgba sidplayfp snesspc sseqplayer unrar v2m vgm vgmstream vio2sf vorbis xmp z80ex zlib

ld_flags += -Wl,--version-script=libzxtune.version

include $(dirs.root)/makefile.mak

install_$(platform):
	$(call copyfile_cmd,$(target),$(DESTDIR))

install_debug:
	$(call copyfile_cmd,$(target).pdb,$(DESTDIR))
