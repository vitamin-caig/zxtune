binary_name := io_test_providers
path_step := ../../../..
source_files := test

include $(path_step)/make/default.mak

$(platform)_include_dirs += $(path_step)/3rdparty

libraries := binary debug io l10n_stub platform tools
depends := src/binary src/debug src/l10n/stub src/platform src/tools src/io

windows_libraries := advapi32
boost_libraries := filesystem system

include $(path_step)/makefile.mak
