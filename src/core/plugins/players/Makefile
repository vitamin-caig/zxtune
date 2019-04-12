library_name := core_plugins_players
dirs.root := ../../../..
source_dirs := . asap ay dac gme multi music saa sid tfm xmp xsf

po_files := core_players

include $(dirs.root)/3rdparty/sidplayfp/defines.mak

include $(dirs.root)/makefile.mak

ifdef tools.python
sid/songlengths_db.inc: sid/Songlengths.txt sid/mksonglength.py
	$(tools.python) sid/mksonglength.py < $< > $@
endif

sid/songlengths.cpp: sid/songlengths_db.inc
