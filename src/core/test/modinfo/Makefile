binary_name := module_info
path_step := ../../../..
source_files := main

libraries := analysis binary core devices formats_archived formats_packed formats_chiptune io l10n_stub platform sound strings tools
depends := $(addprefix src/,analysis binary core devices formats/archived formats/packed formats/chiptune io l10n/stub platform sound strings tools)

#3rdparty
libraries += lhasa z z80ex
depends += 3rdparty/lhasa/lib 3rdparty/zlib 3rdparty/z80ex

boost_libraries += filesystem system

windows_libraries := advapi32

include $(path_step)/makefile.mak
