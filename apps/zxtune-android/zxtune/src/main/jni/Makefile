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

libraries.3rdparty = asap gme he ht hvl lazyusf2 lhasa lzma mgba sidplayfp snesspc unrar vio2sf xmp z80ex zlib

include $(dirs.root)/makefile.mak

zxtune.h: ../java/app/zxtune/ZXTune.java
	javah -classpath ../java -o $@ app.zxtune.ZXTune

jni_plugin.h: ../java/app/zxtune/core/jni/Plugins.java
	javah -classpath ../java -o $@ app.zxtune.core.jni.Plugins

install_$(platform):
	$(call copyfile_cmd,$(target),$(DESTDIR))

install_debug:
	$(call copyfile_cmd,$(target).pdb,$(DESTDIR))
