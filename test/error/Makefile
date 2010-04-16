binary_name := error_test
path_step := ../..
source_files := test

libraries := tools
depends := src/tools

ifdef dynamic_test
definitions := DYNAMIC_ERROR_TEST
dynamic_libs := error_dynamic
endif

windows_libraries := delayimp.lib

include $(path_step)/makefile.mak
