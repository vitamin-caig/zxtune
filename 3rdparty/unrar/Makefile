library_name := unrar
dirs.root := ../..
include $(dirs.root)/make/default.mak

#SILENT - disables all output
#RAR_NOCRYPT - disables encryption support
#SHELL_EXT - disables encryption and some output
#NOVOLUME - disables volues support
#GUI - disables some console i/o
defines += SILENT RAR_NOCRYPT GUI SHELL_EXT NOVOLUME

#core files
source_files = getbits unpack rarvm crc smallfn

#system-related
source_files += system global

#io-related
source_files += rdwrfn

#stubs names should be differ from real files
source_dirs += stubs

source_files := $(addsuffix .cpp,$(source_files))

include $(dirs.root)/makefile.mak
