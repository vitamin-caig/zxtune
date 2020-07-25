binary_name := resource_test
dirs.root := ../../..
source_dirs := .
embedded_files := resource.zip test.cpp

libraries.common = binary binary_compression binary_format debug formats_archived formats_chiptune formats_packed l10n_stub platform resource strings tools
libraries.3rdparty = lhasa lzma unrar zlib

include $(dirs.root)/makefile.mak

ifdef tools.root
resource.zip: file nested/dir/file
		$(tools.root)zip -9J $@ $^
endif
