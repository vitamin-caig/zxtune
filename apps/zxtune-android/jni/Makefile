dynamic_name := zxtune
path_step := ../../..

source_dirs := .

libraries.common = analysis \
                   binary binary_format \
                   core core_plugins_archives_stub core_plugins_containers_stub core_plugins_players \
                   devices_aym devices_dac devices_fm devices_saa devices_z80 \
                   formats_chiptune formats_packed_lha \
                   parameters \
                   sound strings \
                   tools

libraries.3rdparty = lhasa sidplayfp xmp z80ex zlib

include $(path_step)/makefile.mak

zxtune.h: ../src/app/zxtune/ZXTune.java
	javah -classpath ../src -o $@ app.zxtune.ZXTune
