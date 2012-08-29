binary_name := l10n_test_boost_locale
path_step := ../../../..
source_files := test
po_files = en/test ru/test

include $(path_step)/make/default.mak

libraries := l10n tools
depends := src/l10n src/tools

boost_libraries := locale
mingw_libraries := iconv

include $(path_step)/makefile.mak
