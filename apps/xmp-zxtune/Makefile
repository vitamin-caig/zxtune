dynamic_name := xmp-zxtune
path_step := ../..
include $(path_step)/make/default.mak

source_dirs := src $(path_step)/src/core/plugins/containers

libraries.common = analysis \
                   binary binary_format formats_archived formats_packed \
                   core core_plugins_archives core_plugins_players \
                   debug devices_aym devices_beeper devices_dac devices_fm devices_saa devices_z80 \
                   formats_chiptune \
                   l10n_stub \
                   parameters platform_version\
                   sound strings \
                   tools

libraries.3rdparty = lhasa sidplayfp snesspc xmp z80ex zlib unrar

have_gui = 1
static_runtime = 1

include_dirs := $(prebuilt.dir)/xmp-sdk

$(platform)_libraries = user32
LD_PLATFORM_FLAGS += /DEF:$(prebuilt.dir)/xmp-sdk/xmpin.def

include $(path_step)/makefile.mak
