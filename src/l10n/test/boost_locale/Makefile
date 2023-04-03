binary_name := l10n_test_boost_locale
dirs.root := ../../../..
source_dirs := .

include $(dirs.root)/make/default.mak

libraries.common = binary binary_compression binary_format debug formats_archived formats_chiptune formats_packed \
                   l10n platform resource strings tools

libraries.3rdparty = lhasa lzma unrar zlib

libraries.boost += locale
libraries.darwin += iconv

include $(dirs.root)/makefile.mak
