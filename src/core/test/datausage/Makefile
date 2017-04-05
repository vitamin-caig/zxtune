binary_name := core_test_datausage
path_step := ../../../..
source_dirs := .

libraries.common = analysis \
                   binary binary_compression binary_format \
                   core core_plugins_archives core_plugins_players \
                   debug devices_aym devices_beeper devices_dac devices_fm devices_saa devices_z80 \
                   formats_archived formats_archived_multitrack formats_chiptune formats_multitrack formats_packed \
                   io \
                   l10n_stub \
                   module module_players \
                   parameters platform \
                   sound strings \
                   tools

#3rdparty
libraries.3rdparty = gme he hvl lazyusf2 lhasa lzma sidplayfp snesspc unrar xmp z80ex zlib

libraries.boost += filesystem system

windows_libraries := advapi32

include $(path_step)/makefile.mak
