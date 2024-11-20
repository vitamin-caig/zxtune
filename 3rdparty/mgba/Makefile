library_name := mgba
dirs.root := ../..

source_files += $(addprefix arm/,arm decoder-arm decoder-thumb isa-arm isa-thumb) \
                $(addprefix core/,cheats config core interface sync tile-cache timing) \
                $(addprefix gb/,gb_audio) \
                $(addprefix gba/,gba_audio gba_core gba_cheats gba gba_bios gba_dma gba_hardware gba_io gba_memory gba_overrides gba_savedata gba_serialize gba_sio gba_timer gba_vfame gba_video hle-bios) \
                $(addprefix gba/cheats/,codebreaker gameshark parv3) \
                third-party/blip_buf/blip_buf \
                third-party/inih/ini \
                $(addprefix util/,circle-buffer configuration crc32 formatting patch patch-ips patch-ups hash string table vfs vfs/vfs-file vfs/vfs-mem)

source_files += platform/portable/memory
                
source_files := $(addprefix src/,$(addsuffix .c,$(source_files)))

defines.linux += HAVE_LOCALTIME_R
includes.dirs += $(dirs.root)/3rdparty/mgba/include $(dirs.root)/3rdparty/mgba/src
includes.files += defines.h

include $(dirs.root)/makefile.mak
