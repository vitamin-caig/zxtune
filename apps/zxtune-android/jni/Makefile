dynamic_name := zxtune
path_step := ../../..

source_dirs := .

libraries = analysis binary core devices devices_aym formats_archived formats_chiptune formats_packed l10n_stub sound strings tools
depends = $(addprefix src/,analysis binary core devices devices/aym formats/archived formats/chiptune formats/packed l10n/stub sound strings tools)

libraries += lhasa z80ex z
depends += $(addprefix 3rdparty/,lhasa/lib z80ex zlib)

android_libraries := log

include $(path_step)/makefile.mak

zxtune.h: ../src/app/zxtune/ZXTune.java
	javah -classpath ../src -o $@ app.zxtune.ZXTune
