library_name := module_players
dirs.root := ../../..
source_dirs := . aym dac saa tfm xsf

po_files := module_players

includes.dirs.gsf.cpp = $(dirs.root)/3rdparty/mgba/include $(dirs.root)/3rdparty/mgba/src
includes.files.gsf.cpp = $(dirs.root)/3rdparty/mgba/defines.h

jumbo.name := $(library_name)
jumbo.excludes := dac/abysshighestexperience.cpp xsf/2sf.cpp xsf/gsf.cpp xsf/ncsf.cpp xsf/psf.cpp xsf/sdsf.cpp xsf/usf.cpp

include $(dirs.root)/makefile.mak

