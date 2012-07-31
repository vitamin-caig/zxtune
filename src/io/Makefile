library_name := io
path_step := ../..
source_dirs := impl
source_files = providers/enumerator

text_files := text/io

include $(path_step)/make/default.mak
include $(path_step)/make/features.mak

#curl
ifdef support_curl
$(platform)_include_dirs += $(path_step)/3rdparty
generated_headers += providers/curl_api
source_files += providers/network_provider providers/curl_api_dynamic
else
source_files += providers/network_stub
endif

source_files += providers/file_provider

include $(path_step)/makefile.mak

%_api_dynamic$(src_suffix): templates/%_api_dynamic$(src_suffix).templ functions/%.dat
	perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@

%_api.h: templates/%_api.h.templ functions/%.dat
	perl $(path_step)/make/build_shgate_dynamic.pl $^ > $@
