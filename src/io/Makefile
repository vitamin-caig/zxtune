library_name := io
path_step := ../..
source_dirs := impl
source_files = providers/enumerator

text_files := text/io
po_files := io

include $(path_step)/make/default.mak
include $(path_step)/make/features.mak

#curl
ifdef support_curl
generated_headers += providers/gates/curl_api
generated_sources += providers/gates/curl_api_dynamic
source_files += providers/network_provider
else
source_files += providers/network_stub
endif

source_files += providers/file_provider

include $(path_step)/makefile.mak
include $(path_step)/make/shgate.mak
