dynamic_name := foo_input_zxtune
path_step := ../..
include $(path_step)/make/default.mak

source_dirs := src $(path_step)/src/core/plugins/containers

libraries.common = analysis \
                   binary binary_format formats_archived formats_archived_multitrack formats_chiptune formats_packed formats_multitrack\
                   core core_plugins_archives core_plugins_players \
                   debug devices_aym devices_beeper devices_dac devices_fm devices_saa devices_z80 \
                   l10n_stub \
                   parameters platform_version\
                   sound strings \
                   tools

libraries.3rdparty = gme hvl lhasa lzma sidplayfp snesspc unrar xmp z80ex zlib

have_gui = 1
static_runtime = 1

ifdef release
FOOBAR_LIB_FOLDER=Release
else
FOOBAR_LIB_FOLDER=Debug
endif

FOOBAR_SDK_DIR := $(prebuilt.dir)/SDK-2015-08-03

include_dirs := $(FOOBAR_SDK_DIR)/foobar2000/SDK

$(platform)_libraries_dirs += $(FOOBAR_SDK_DIR)/foobar2000/SDK/$(FOOBAR_LIB_FOLDER)
$(platform)_libraries_dirs += $(FOOBAR_SDK_DIR)/foobar2000/foobar2000_component_client/$(FOOBAR_LIB_FOLDER)
$(platform)_libraries_dirs += $(FOOBAR_SDK_DIR)/foobar2000/shared
$(platform)_libraries_dirs += $(FOOBAR_SDK_DIR)/pfc/$(FOOBAR_LIB_FOLDER)

$(platform)_libraries = foobar2000_SDK foobar2000_component_client shared pfc user32

include $(path_step)/makefile.mak
