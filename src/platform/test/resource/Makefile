binary_name := platform_test_resource
path_step := ../../../..
source_files := test
embedded_files := resource.zip test.cpp

libraries := binary formats l10n_stub platform tools z
depends := src/binary src/formats src/l10n/stub src/platform src/tools 3rdparty/zlib

include $(path_step)/makefile.mak

resource.zip: Makefile nested/dir/file
		zip -9J $@ $^
