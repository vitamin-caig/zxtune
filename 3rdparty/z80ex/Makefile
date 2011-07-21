library_name := z80ex
path_step := ../..
include $(path_step)/make/default.mak

API_REVISION=1
VERSION_MAJOR=1
VERSION_MINOR=19
RELEASE_TYPE =
VERSION_STR = ${API_REVISION}.${VERSION_MAJOR}.${VERSION_MINOR}${RELEASE_TYPE}

$(platform)_definitions += Z80EX_OPSTEP_FAST_AND_ROUGH\
 Z80EX_VERSION_STR=${VERSION_STR} Z80EX_API_REVISION=${API_REVISION}\
 Z80EX_VERSION_MAJOR=${VERSION_MAJOR} Z80EX_VERSION_MINOR=${VERSION_MINOR}\
 Z80EX_RELEASE_TYPE=${RELEASE_TYPE}

include_dirs = . include
source_files_cc := z80ex

include $(path_step)/makefile.mak
