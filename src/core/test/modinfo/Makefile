binary_name := module_info
path_step := ../../../..
source_dirs := .

libraries.common = analysis \
                   binary binary_format \
                   core core_plugins_archives_stub core_plugins_containers_stub core_plugins_players \
                   debug devices_aym devices_beeper devices_dac devices_fm devices_saa devices_z80 \
                   formats_chiptune formats_packed_lha \
                   io \
                   l10n_stub \
                   parameters platform \
                   sound strings \
                   tools

#3rdparty
libraries.3rdparty = lhasa sidplayfp sidplayfp_builders_resid xmp z80ex zlib

libraries.boost += filesystem system

windows_libraries := advapi32

include $(path_step)/makefile.mak
