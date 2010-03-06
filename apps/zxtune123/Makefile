binary_name := zxtune123
path_step := ../..
include $(path_step)/default.mak
source_files := main parsing information sound source cli_app
ifeq ($(platform),mingw)
source_files += console_windows
else ifeq ($(platform),dingux)
source_files += console_linux
else
source_files += console_$(platform)
endif

text_files := text
other_text_files := cmdline messages conversion

libraries := core sound tools io texts
depends := text src/tools src/core src/sound src/io

windows_libraries := advapi32
boost_libraries := thread program_options

include $(path_step)/makefile.mak
