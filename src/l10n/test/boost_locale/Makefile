binary_name := l10n_test_boost_locale
dirs.root := ../../../..
source_dirs := .

include $(dirs.root)/make/default.mak

libraries.common = debug l10n tools

libraries.boost += locale
libraries.mingw += iconv
libraries.darwin += iconv

include $(dirs.root)/makefile.mak
