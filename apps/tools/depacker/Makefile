binary_name := depacker
dirs.root := ../../..
source_dirs := .

libraries.common = binary binary_compression binary_format debug formats_chiptune formats_packed strings tools

libraries.3rdparty = lhasa zlib

include $(dirs.root)/make/default.mak

ifdef jumbo.enabled
libraries.common += formats_chiptune strings
libraries.3rdparty += unrar
endif

include $(dirs.root)/makefile.mak
