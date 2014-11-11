binary_name := resource_test
path_step := ../../..
source_dirs := .
embedded_files := resource.zip test.cpp

libraries.common = binary binary_format debug formats_archived formats_packed l10n_stub platform resource tools
libraries.3rdparty = zlib

include $(path_step)/makefile.mak

ifdef tools.root
resource.zip: file nested/dir/file
		$(tools.root)zip -9J $@ $^
endif
