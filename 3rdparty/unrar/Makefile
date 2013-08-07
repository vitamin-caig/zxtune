library_name := unrar
path_step := ../..
include $(path_step)/make/default.mak

#SILENT - disables all output
#RAR_NOCRYPT - disables encryption support
#SHELL_EXT - disables encryption and some output
#NOVOLUME - disables volues support
#GUI - disables some console i/o
$(platform)_definitions += SILENT RAR_NOCRYPT GUI SHELL_EXT NOVOLUME

#core files
source_files = getbits unpack rarvm crc smallfn

#system-related
source_files += system global

#io-related
source_files += rdwrfn

#stubs names should be differ from real files
source_dirs += stubs

include $(path_step)/makefile.mak
