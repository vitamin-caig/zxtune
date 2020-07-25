library_name := io
dirs.root := ../..
source_dirs := impl
source_files = providers/enumerator.cpp

text_files := text/io
po_files := io

include $(dirs.root)/make/default.mak
include $(dirs.root)/make/features.mak

#curl
ifdef support_curl
generated_headers += providers/gates/curl_api.h
generated_sources += providers/gates/curl_api_dynamic.cpp
source_files += providers/network_provider.cpp
else
source_files += providers/network_stub.cpp
endif

source_files += providers/file_provider.cpp

jumbo.name := $(library_name)
jumbo.for_generated = 1
jumbo.excludes := text/io.cpp

include $(dirs.root)/makefile.mak
include $(dirs.root)/make/shgate.mak
