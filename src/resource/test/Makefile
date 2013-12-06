binary_name := resource_test
path_step := ../../..
source_files := test
embedded_files := resource.zip test.cpp

libraries.common = binary binary_format debug formats_archived formats_packed l10n_stub platform resource tools
libraries.3rdparty = zlib

include $(path_step)/makefile.mak

resource.zip: Makefile nested/dir/file
		$(tools.root)zip -9J $@ $^
