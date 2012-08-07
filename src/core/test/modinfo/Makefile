binary_name := module_info
path_step := ../../../..
source_files := main

libraries := analysis binary core devices formats io sound tools
depends := src/analysis src/binary src/core src/devices src/formats src/io src/sound src/tools

#3rdparty
libraries += lhasa z z80ex
depends += 3rdparty/lhasa/lib 3rdparty/zlib 3rdparty/z80ex

windows_libraries := advapi32

include $(path_step)/makefile.mak
