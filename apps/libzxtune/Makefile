dynamic_name := zxtune
path_step := ../..
source_dirs := src

static_runtime=1

libraries.common = analysis \
                   binary binary_format \
                   core core_plugins_archives_stub core_plugins_containers_stub core_plugins_players \
                   debug devices_aym devices_dac devices_fm devices_saa devices_z80 \
                   formats_chiptune formats_packed_lha \
                   l10n_stub \
                   parameters platform_version \
                   sound strings \
                   tools

libraries.3rdparty = lhasa xmp z80ex zlib sidplayfp sidplayfp_builders_resid

windows_libraries := oldnames

include $(path_step)/makefile.mak
