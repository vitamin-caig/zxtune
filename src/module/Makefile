library_name := module
dirs.root := ../..
source_dirs := players players/aym players/dac players/saa players/tfm players/xsf

po_files := module

includes.dirs.gsf.cpp = $(dirs.root)/3rdparty/mgba/include $(dirs.root)/3rdparty/mgba/src
includes.files.gsf.cpp = $(dirs.root)/3rdparty/mgba/defines.h

jumbo.name := $(library_name)
jumbo.excludes := players/dac/abysshighestexperience.cpp players/xsf/2sf.cpp players/xsf/gsf.cpp players/xsf/ncsf.cpp players/xsf/psf.cpp players/xsf/sdsf.cpp players/xsf/usf.cpp

include $(dirs.root)/makefile.mak
