binary_name := zxtune123
path_step := ../..
include $(path_step)/default.mak
source_files := information sound source display cli_app
ifeq ($(platform),mingw)
source_files += console_windows
else
source_files += console_$(platform)
endif

text_files := text/text
other_text_files := text/cmdline text/messages text/conversion text/cmd_keys_local

libraries := core devices sound tools io zxtune_app_base
depends := src/tools src/core src/devices src/sound src/io apps/base

windows_libraries := advapi32
boost_libraries := thread program_options

manual_format := txt
pkg_additional_files = ../zxtune.conf $(output_dir)/$(binary_name)_$(pkg_lang).$(manual_format)
pkg_additional_files_dingux := dist/dingux/menuitem.cfg dist/dingux/readme.txt dist/dingux/zxtune.png

include $(path_step)/makefile.mak

$(output_dir)/$(binary_name)_$(pkg_lang).$(manual_format): dist/$(binary_name).txt | $(output_dir)
	$(TEXTATOR) --process --keys $(pkg_lang),$(manual_format) --asm --output $@ $<
