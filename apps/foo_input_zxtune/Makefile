dynamic_name := foo_input_zxtune
path_step := ../..
include $(path_step)/make/default.mak

source_dirs := src

libraries.common = analysis \
                   binary binary_format formats_archived formats_packed\
                   core core_plugins_archives core_plugins_containers core_plugins_players \
                   debug devices_aym devices_dac devices_fm devices_saa devices_z80 \
                   formats_chiptune formats_packed_lha \
                   l10n_stub \
                   parameters \
                   sound strings \
                   tools

libraries.3rdparty = lhasa xmp z80ex zlib unrar sidplayfp sidplayfp_builders_resid

windows_libraries := oldnames

have_gui = 1
static_runtime = 1

include_dirs := $(prebuilt.dir)/SDK-2011-03-11/foobar2000/SDK

$(platform)_libraries_dirs += $(prebuilt.dir)/SDK-2011-03-11/foobar2000/SDK/Release
$(platform)_libraries_dirs += $(prebuilt.dir)/SDK-2011-03-11/foobar2000/foobar2000_component_client/Release
$(platform)_libraries_dirs += $(prebuilt.dir)/SDK-2011-03-11/foobar2000/shared
$(platform)_libraries_dirs += $(prebuilt.dir)/SDK-2011-03-11/pfc/Release

$(platform)_libraries = foobar2000_SDK foobar2000_component_client shared pfc user32

include $(path_step)/makefile.mak
