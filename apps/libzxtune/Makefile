dynamic_name := zxtune
path_step := ../..
source_dirs := src

libraries.common = analysis \
                   binary \
                   core core_plugins_archives_stub core_plugins_containers_stub core_plugins_players \
                   debug devices_aym devices_dac devices_fm devices_saa devices_z80 \
                   formats_chiptune formats_packed_lha \
                   l10n_stub \
                   parameters \
                   sound strings \
                   tools

libraries.3rdparty = lhasa z80ex

libraries :=  zxtune_app_version
depends := apps/version

windows_libraries := oldnames

include $(path_step)/makefile.mak
