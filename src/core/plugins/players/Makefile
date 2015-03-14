library_name := core_plugins_players
path_step := ../../../..
source_dirs := . ay dac gme saa sid tfm xmp

include $(path_step)/3rdparty/sidplayfp/defines.mak

include $(path_step)/makefile.mak

ifdef tools.python
sid/songlengths_db.inc: sid/Songlengths.txt sid/mksonglength.py
	$(tools.python) sid/mksonglength.py < $< > $@
endif

sid/songlengths.cpp: sid/songlengths_db.inc
