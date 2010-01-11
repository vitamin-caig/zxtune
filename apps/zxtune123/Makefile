binary_name := zxtune123
path_step := ../..
source_files := main parsing information sound source
text_files := messages cmdline

source_files += cli_app console_$(platform)

libraries := core sound tools io texts
depends := text src/tools src/core src/sound src/io

windows_libraries := advapi32
boost_libraries := thread program_options

include $(path_step)/makefile.mak
