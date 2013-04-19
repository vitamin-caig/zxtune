dynamic_name := zxtune
path_step := ../../..

source_dirs := .

libraries.common = analysis \
                   binary \
                   core core_plugins_archives core_plugins_containers core_plugins_players \
                   devices devices_aym \
                   formats_archived formats_chiptune formats_packed \
                   l10n_stub sound strings \
                   tools

libraries.3rdparty = lhasa unrar z80ex zlib

android_libraries := log

include $(path_step)/makefile.mak

zxtune.h: ../src/app/zxtune/ZXTune.java
	javah -classpath ../src -o $@ app.zxtune.ZXTune
