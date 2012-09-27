binary_name := io_test_providers
path_step := ../../../..
source_files := test

include $(path_step)/make/default.mak

$(platform)_include_dirs += $(path_step)/3rdparty

libraries := binary io l10n_stub tools
depends := src/binary src/l10n/stub src/tools src/io

windows_libraries := advapi32
boost_libraries := filesystem system

include $(path_step)/makefile.mak
