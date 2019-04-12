binary_name := io_test_providers
dirs.root := ../../../..
source_dirs := .

include $(dirs.root)/make/default.mak

libraries.common = binary debug io l10n_stub platform strings tools

libraries.windows := advapi32
libraries.boost := filesystem system

include $(dirs.root)/makefile.mak
