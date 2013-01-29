dynamic_name := zxtune
path_step := ../..
source_dirs := src

libraries := analysis binary core debug devices devices_aym formats_archived formats_packed formats_chiptune io l10n_stub platform sound strings tools zxtune_app_version
depends := $(addprefix src/,analysis binary core debug devices devices/aym formats/archived formats/packed formats/chiptune io l10n/stub platform sound strings tools) apps/version

#3rdparty
libraries += lhasa z z80ex
depends += 3rdparty/lhasa/lib 3rdparty/z80ex 3rdparty/zlib

boost_libraries += filesystem system

windows_libraries := advapi32 oldnames

include $(path_step)/makefile.mak
