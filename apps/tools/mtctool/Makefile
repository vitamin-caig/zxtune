binary_name := mtctool
dirs.root := ../../..
source_dirs := .

libraries.common = binary binary_format debug formats_chiptune module platform_version strings tools

include $(dirs.root)/make/default.mak

ifdef jumbo.enabled
libraries.common += binary_compression formats_packed
libraries.3rdparty = lhasa unrar zlib
endif

include $(dirs.root)/makefile.mak
