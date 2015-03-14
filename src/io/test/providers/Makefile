binary_name := io_test_providers
path_step := ../../../..
source_dirs := .

include $(path_step)/make/default.mak

libraries.common = binary debug io l10n_stub platform tools

windows_libraries := advapi32
libraries.boost := filesystem system

include $(path_step)/makefile.mak
