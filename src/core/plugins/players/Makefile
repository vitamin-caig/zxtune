library_name := core_plugins_players
path_step := ../../../..
source_dirs := . ay dac saa sid tfm xmp

include $(path_step)/3rdparty/sidplayfp/defines.mak

include $(path_step)/makefile.mak

ifdef tools.perl
sid/songlengths_db.h: sid/Songlengths.txt sid/mksonglength.pl
	$(tools.perl) sid/mksonglength.pl < $< > $@
endif

