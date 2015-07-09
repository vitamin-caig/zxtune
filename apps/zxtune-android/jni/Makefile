dynamic_name := zxtune
path_step := ../../..

source_dirs := .

libraries.common = analysis \
                   binary binary_format \
                   core core_plugins_archives_lite core_plugins_containers_lite core_plugins_players \
                   devices_aym devices_beeper devices_dac devices_fm devices_saa devices_z80 \
                   formats_chiptune formats_packed_archives formats_packed_decompilers formats_archived formats_archived_multitrack formats_multitrack \
                   parameters \
                   sound strings \
                   tools

libraries.3rdparty = gme lhasa lzma sidplayfp snesspc unrar xmp z80ex zlib

include $(path_step)/makefile.mak

zxtune.h: ../src/app/zxtune/ZXTune.java
	javah -classpath ../src -o $@ app.zxtune.ZXTune

install_$(platform):
	$(call copyfile_cmd,$(target),$(DESTDIR))

install_debug:
	$(call copyfile_cmd,$(target).pdb,$(DESTDIR))
