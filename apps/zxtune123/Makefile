binary_name := zxtune123
path_step := ../..
include $(path_step)/make/default.mak

source_files := information sound source display cli_app
ifeq ($(platform),mingw)
source_files += console_windows
else ifeq ($(platform),macos)
source_files += console_linux
else
source_files += console_$(platform)
endif

text_files := text/text
other_text_files := text/cmdline text/messages text/conversion text/cmd_keys_local

libraries := async core devices formats sound tools io zxtune_app_base
depends := src/async src/tools src/core src/devices src/formats src/sound src/io apps/base

windows_libraries = advapi32
boost_libraries = thread program_options

manual_format := txt
manual_file = $(binary_name)_$(pkg_lang).$(manual_format)
pkg_additional_files = ../zxtune.conf
ifeq ($(NO_TEXTATOR),)
pkg_additional_files += $(output_dir)/$(manual_file)
endif
pkg_additional_files_dingux := $(wildcard dist/dingux/dmenu/*) $(wildcard dist/dingux/gmenu/*) dist/dingux/readme.txt dist/dingux/zxtune.png

include $(path_step)/makefile.mak
include $(path_step)/make/package.mak

install: $(output_dir)/$(manual_file)
	install -D $(target) $(DESTDIR)/usr/bin/$(binary_name)
	install -m644 -D ../zxtune.conf $(DESTDIR)/usr/share/zxtune/zxtune.conf
	install -m644 -D $(output_dir)/$(manual_file) $(DESTDIR)/usr/share/zxtune/$(manual_file)

$(output_dir)/$(manual_file): dist/$(binary_name).txt | $(output_dir)
	$(TEXTATOR) --process --keys $(pkg_lang),$(manual_format) --asm --output $@ $<
