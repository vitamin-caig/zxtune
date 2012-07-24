dynamic_name := zxtune
path_step := ../..
source_dirs := src

libraries := analysis binary core devices formats io sound tools zxtune_app_version
depends := src/analysis src/binary src/core src/devices src/formats src/io src/sound src/tools apps/version

#3rdparty
libraries += lhasa z z80ex
depends += 3rdparty/lhasa/lib 3rdparty/z80ex 3rdparty/zlib

windows_libraries := advapi32

include $(path_step)/makefile.mak
