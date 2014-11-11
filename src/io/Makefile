library_name := io
path_step := ../..
source_dirs := impl
source_files = providers/enumerator.cpp

text_files := text/io
po_files := io

include $(path_step)/make/default.mak
include $(path_step)/make/features.mak

#curl
ifdef support_curl
generated_headers += providers/gates/curl_api.h
generated_sources += providers/gates/curl_api_dynamic.cpp
source_files += providers/network_provider.cpp
else
source_files += providers/network_stub.cpp
endif

source_files += providers/file_provider.cpp

include $(path_step)/makefile.mak
include $(path_step)/make/shgate.mak
