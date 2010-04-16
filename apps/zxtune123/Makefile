binary_name := zxtune123
path_step := ../..
include $(path_step)/default.mak
source_files := information sound source cli_app
ifeq ($(platform),mingw)
source_files += console_windows
else
source_files += console_$(platform)
endif

text_files := text/text
other_text_files := text/cmdline text/messages text/conversion

libraries := core sound tools io zxtune_app_base
depends := src/tools src/core src/sound src/io apps/base

windows_libraries := advapi32
boost_libraries := thread program_options

include $(path_step)/makefile.mak
