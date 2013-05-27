binary_name := module_info
path_step := ../../../..
source_files := main

libraries.common = analysis \
                   binary \
                   core core_plugins_archives_stub core_plugins_containers_stub core_plugins_players \
                   debug devices devices_aym devices_saa \
                   formats_chiptune formats_packed_lha \
                   io \
                   l10n_stub \
                   platform \
                   sound strings \
                   tools

#3rdparty
libraries.3rdparty = lhasa z80ex

libraries.boost += filesystem system

windows_libraries := advapi32

include $(path_step)/makefile.mak
