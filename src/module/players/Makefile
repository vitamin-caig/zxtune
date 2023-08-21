library_name := module_players
dirs.root := ../../..
source_dirs := . aym dac saa tfm xsf

po_files := module_players

include $(dirs.root)/3rdparty/mgba/defines.mak

jumbo.name := $(library_name)
jumbo.excludes := dac/abysshighestexperience.cpp xsf/2sf.cpp xsf/ncsf.cpp

include $(dirs.root)/makefile.mak

ifdef tools.python
aym/sc68_replayers.inc: $(wildcard aym/sc68_replayers/*)
	$(tools.python) $(dirs.root)/make/tools/make_binaries.py $^ > $@
endif
