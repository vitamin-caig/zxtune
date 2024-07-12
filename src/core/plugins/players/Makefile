library_name := core_plugins_players
dirs.root := ../../../..
source_dirs := . asap ay dac gme mpt multi music saa sid tfm vgm vgmstream xmp xsf

po_files := core_players

includes.dirs.sid_base.cpp = $(dirs.root)/3rdparty/sidplayfp
includes.dirs.ffmpeg_decoder.cpp = $(dirs.root)/3rdparty/ffmpeg
includes.dirs.vgmstream_base.cpp = $(dirs.root)/3rdparty/vgmstream/ext_includes
includes.dirs.ogg_supp.cpp = $(dirs.root)/3rdparty

jumbo.name := $(library_name)
jumbo.excludes := asap/asap_base.cpp \
                  gme/spc_supp.cpp gme/gme_base.cpp \
                  sid/sid_base.cpp \
                  mpt/openmpt_base.cpp \
                  music/ffmpeg_decoder.cpp music/flac_supp.cpp music/mp3_supp.cpp music/ogg_supp.cpp \
                  vgm/vgm_base.cpp \
                  vgmstream/vgmstream_base.cpp \
                  xmp/xmp_base.cpp

include $(dirs.root)/makefile.mak

ifdef tools.python
sid/songlengths_db.inc: sid/mksonglength.py $(sort $(wildcard sid/Songlengths*.md5))
	$(tools.python) $^ > $@
endif

sid/songlengths.cpp: sid/songlengths_db.inc
