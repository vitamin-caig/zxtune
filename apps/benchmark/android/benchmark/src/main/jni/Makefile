dynamic_name := benchmark
path_step := ../../../../../../..
source_dirs := .

libraries = benchmark 
libraries.common = devices_aym devices_z80 l10n_stub sound tools
libraries.3rdparty = z80ex

depends := apps/benchmark/core

android_libraries := log

include $(path_step)/makefile.mak

install_$(platform):
	$(call copyfile_cmd,$(target),$(DESTDIR))

install_debug:
	$(call copyfile_cmd,$(target).pdb,$(DESTDIR))
