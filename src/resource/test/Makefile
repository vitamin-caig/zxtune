binary_name := resource_test
path_step := ../../..
source_files := test
embedded_files := resource.zip test.cpp

libraries := binary formats_archived formats_packed l10n_stub platform resource tools z
depends := src/binary src/formats/archived src/formats/packed src/l10n/stub src/platform src/resource src/tools 3rdparty/zlib

include $(path_step)/makefile.mak

resource.zip: Makefile nested/dir/file
		zip -9J $@ $^
