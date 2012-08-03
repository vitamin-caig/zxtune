binary_name := io_test_providers
path_step := ../../../..
source_files := test

include $(path_step)/make/default.mak

$(platform)_include_dirs += $(path_step)/3rdparty

libraries := binary io tools
depends := src/binary src/tools src/io

windows_libraries := advapi32

include $(path_step)/makefile.mak
