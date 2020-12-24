binary_name := psfdump
dirs.root := ../../..
source_dirs := .

libraries.common = binary binary_compression binary_format debug formats_chiptune strings tools

libraries.3rdparty = zlib

libraries.windows += advapi32

include $(dirs.root)/make/default.mak

ifdef jumbo.enabled
libraries.common += formats_packed_lha
libraries.3rdparty += lhasa unrar
endif

include $(dirs.root)/makefile.mak
