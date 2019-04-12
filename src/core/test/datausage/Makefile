binary_name := core_test_datausage
dirs.root := ../../../..
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
libraries.3rdparty = asap gme he ht hvl lazyusf2 lhasa lzma mgba sidplayfp snesspc unrar vio2sf xmp z80ex zlib

libraries.boost += filesystem system

libraries.windows := advapi32 oldnames

include $(dirs.root)/makefile.mak
